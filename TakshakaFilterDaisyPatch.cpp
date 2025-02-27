#include "daisy_patch.h"
#include "daisysp.h"
#include "Filters/moogladder.h"
#include "Filters/comb.h"
#include "compressor.h"

#define SAMPLE_RATE 48000.f
#define COMB_BUFFER_SIZE 9600
#define MAX_INCREMENT 20

using namespace daisy;
using namespace daisysp;

enum menus { MENU_TOP, MENU_VENOM, MENU_FILTER_ORDER, MENU_COMP, MENUS_COUNT };
enum menuModes { MENU_MODE_SELECT, MENU_MODE_EDIT, MENU_MODES_COUNT };
enum filterOrders { FILTER_ORDER_LADDER_FIRST, FILTER_ORDER_COMB_FIRST, FILTER_ORDERS_COUNT };
enum compSettings { COMP_ATTACK, COMP_MAKEUP, COMP_RATIO, COMP_RELEASE, COMP_THRESHOLD, COMP_SETTINGS_COUNT };
enum curves { CURVE_LINEAR, CURVE_EXPONENTIAL, CURVE_LOGARITHMIC, CURVE_CUBE, CURVES_COUNT };

DaisyPatch hw;
MoogLadder ladder;
Comb comb;
Compressor comp;
Parameter growlKnob, howlKnob, resKnob, fdbkKnob;

bool isCompSubmenu = false, 
	updateDisplay = true;

float combBuffer[ COMB_BUFFER_SIZE ],
	currentVenomValue,
	compAttackMin = 0.01f,
	compMakeupMin = 0.f,
	compRatioMin = 1.f,
	compReleaseMin = 0.01f,
	compThresholdMin = 0.f,
	compAttackMax = 10.f,
	compMakeupMax = 80.f,
	compRatioMax = 40.f,
	compReleaseMax = 10.f,
	compThresholdMax = -80.f,
	venomMin = 0.f,
	venomMax = 80.f;

int currentMenu = MENU_TOP,
	currentTopMenuSetting = MENU_VENOM,
	currentCompSetting = COMP_ATTACK,
	currentFilterOrder = FILTER_ORDER_LADDER_FIRST,
	currentCompAttackIncrement = 1,
	currentCompMakeupIncrement = 1,
	currentCompRatioIncrement = 1,
	currentCompReleaseIncrement = 1,
	currentCompThresholdIncrement = 1,
	currentVenomIncrement = 1;

float processCurve( float in, float min, float max, int curveType ){
	float output = 0.f;
	switch( curveType ){
        case CURVE_LINEAR: output = (in * (max - min)) + min; break;
        case CURVE_EXPONENTIAL: output = ((in * in) * (max - min)) + min; break;
        case CURVE_LOGARITHMIC: output = expf((in * (max - min)) + min); break;
        case CURVE_CUBE: output = ((in * (in * in)) * (max - min)) + min; break;
        default: break;
    }
    return output;
}
void updateCurrentCompAttack(){
	comp.SetAttack( 
			processCurve(
			(float) ( currentCompAttackIncrement - 1 ) / (float) ( MAX_INCREMENT - 1 ),
			compAttackMin,
			compAttackMax,
			CURVE_EXPONENTIAL
		) 
	);
}
void updateCurrentCompMakeup(){
		comp.SetMakeup(  
			processCurve(
			(float) ( currentCompMakeupIncrement - 1 ) / (float) ( MAX_INCREMENT - 1 ),
			compMakeupMin,
			compMakeupMax,
			CURVE_EXPONENTIAL
		)
 	);
}
void updateCurrentCompRatio(){	
	comp.SetRatio( 
		processCurve(
			(float) ( currentCompRatioIncrement - 1 ) / (float) ( MAX_INCREMENT - 1 ),
			compRatioMin,
			compRatioMax,
			CURVE_LINEAR
		)
	 );
}
void updateCurrentCompRelease(){
	comp.SetRelease( 
		processCurve(
			(float) ( currentCompReleaseIncrement - 1 ) / (float) ( MAX_INCREMENT - 1 ),
			compReleaseMin,
			compReleaseMax,
			CURVE_EXPONENTIAL
		)
	 );
}
void updateCurrentCompThreshold(){
	comp.SetThreshold( 
			processCurve(
			(float) ( currentCompThresholdIncrement - 1 ) / (float) ( MAX_INCREMENT - 1 ),
			compThresholdMin,
			compThresholdMax,
			CURVE_EXPONENTIAL
		)
	);
}
void updateCurrentCompSettingValue(){
	switch( currentCompSetting ){
		case COMP_ATTACK:
			updateCurrentCompAttack();
			break;
		case COMP_MAKEUP:
			updateCurrentCompMakeup();
			break;
		case COMP_RATIO:
			updateCurrentCompRatio();
			break;
		case COMP_RELEASE:
			updateCurrentCompRelease();
			break;
		case COMP_THRESHOLD:
			updateCurrentCompThreshold();
			break;
		default: break;
	}
}
void handleCompSubMenuEncoderUp(){
	switch( currentCompSetting ){
		case COMP_ATTACK:
			currentCompAttackIncrement++;
			if( currentCompAttackIncrement > MAX_INCREMENT ) currentCompAttackIncrement = MAX_INCREMENT;
			break;
		case COMP_MAKEUP:
			currentCompMakeupIncrement++;
			if( currentCompMakeupIncrement > MAX_INCREMENT ) currentCompMakeupIncrement = MAX_INCREMENT;
			break;
		case COMP_RATIO:
			currentCompRatioIncrement++;
			if( currentCompRatioIncrement > MAX_INCREMENT ) currentCompRatioIncrement = MAX_INCREMENT;
			break;
		case COMP_RELEASE:
			currentCompReleaseIncrement++;
			if( currentCompReleaseIncrement > MAX_INCREMENT ) currentCompReleaseIncrement = MAX_INCREMENT;
			break;
		case COMP_THRESHOLD:
			currentCompThresholdIncrement++;
			if( currentCompThresholdIncrement > MAX_INCREMENT ) currentCompThresholdIncrement = MAX_INCREMENT;
			break;
		default: break;
	}
	updateCurrentCompSettingValue();
}
void handleCompSubMenuEncoderDown(){
	switch( currentCompSetting ){
		case COMP_ATTACK:
			currentCompAttackIncrement--;
			if( currentCompAttackIncrement < 1 ) currentCompAttackIncrement = 1;
			break;
		case COMP_MAKEUP:
			currentCompMakeupIncrement--;
			if( currentCompMakeupIncrement < 1 ) currentCompMakeupIncrement = 1;
			break;
		case COMP_RATIO:
			currentCompRatioIncrement--;
			if( currentCompRatioIncrement < 1 ) currentCompRatioIncrement = 1;
			break;
		case COMP_RELEASE:
			currentCompReleaseIncrement--;
			if( currentCompReleaseIncrement < 1 ) currentCompReleaseIncrement = 1;
			break;
		case COMP_THRESHOLD:
			currentCompThresholdIncrement--;
			if( currentCompThresholdIncrement < 1 ) currentCompThresholdIncrement = 1;
			break;
		default: break;
	}
	updateCurrentCompSettingValue();
}
void handleCompMenuEncoderUp() {
	if( isCompSubmenu ) handleCompSubMenuEncoderUp();
	else {
		currentCompSetting++;
		if( currentCompSetting >= COMP_SETTINGS_COUNT ) currentCompSetting = COMP_SETTINGS_COUNT - 1;
	}
}
void handleCompMenuEncoderDown() {
	if( isCompSubmenu ) handleCompSubMenuEncoderDown();
	else {
		currentCompSetting--;
		if( currentCompSetting < -1 ) currentCompSetting = -1;
	}
}
void updateVenomValue(){
	currentVenomValue = processCurve(
		(float) (currentVenomIncrement - 1) / (float) (MAX_INCREMENT - 1 ),
		venomMin,
		venomMax,
		CURVE_LINEAR
	);
}
void encoderUp(){
	switch( currentMenu ){
		case MENU_TOP:
			currentTopMenuSetting++;
			if( currentTopMenuSetting >= MENUS_COUNT ) currentTopMenuSetting = MENUS_COUNT - 1;
			break;
		case MENU_VENOM:
			currentVenomIncrement++;
			if( currentVenomIncrement > MAX_INCREMENT) currentVenomIncrement = MAX_INCREMENT;
			updateVenomValue();
			break;
		case MENU_FILTER_ORDER:
			currentFilterOrder++;
			if( currentFilterOrder >= FILTER_ORDERS_COUNT ) currentFilterOrder = FILTER_ORDER_COMB_FIRST;
			break;
		case MENU_COMP:
			handleCompMenuEncoderUp();
			break;
		default: break;
	}
	updateDisplay = true;
}
void encoderDown(){
	switch( currentMenu ){
		case MENU_TOP:
			currentTopMenuSetting--;
			if( currentTopMenuSetting <= MENU_TOP ) currentTopMenuSetting = MENU_VENOM;
			break;
		case MENU_VENOM:
			currentVenomIncrement--;
			if( currentVenomIncrement < 1 ) currentVenomIncrement = 1;
			updateVenomValue();
			break;
		case MENU_FILTER_ORDER:
			currentFilterOrder--;
			if( currentFilterOrder < FILTER_ORDER_LADDER_FIRST ) currentFilterOrder = FILTER_ORDER_LADDER_FIRST;
			break;
		case MENU_COMP:
			handleCompMenuEncoderDown();
			break;
		default: break;
	}
	updateDisplay = true;
}
void encoderClick(){
	switch( currentMenu ){
		case MENU_TOP:
			currentMenu = currentTopMenuSetting;
			break;
		case MENU_VENOM:
		case MENU_FILTER_ORDER:
			currentMenu = MENU_TOP;
			break;
		case MENU_COMP:
			if( isCompSubmenu )isCompSubmenu = false;
			else {
				if( currentCompSetting == -1 ){
					currentMenu = MENU_TOP;
					currentCompSetting = COMP_ATTACK;
				} else isCompSubmenu = true; 
			}
			break;
		default: break;
	}
	updateDisplay = true;
}
void handleEncoder(){
	int increment = hw.encoder.Increment();
	if( increment == 1 ) encoderUp();
	if ( increment == -1 ) encoderDown();
	if( hw.encoder.RisingEdge() ) encoderClick();
}
void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size){
	for (size_t i = 0; i < size; i++){
		out[0][i] = currentFilterOrder == FILTER_ORDER_COMB_FIRST?
			comp.Process( ladder.Process( comb.Process( SoftClip( in[0][i] * ( 1.f + currentVenomValue ) ) ) ) ) : 
			comb.Process( comp.Process( ladder.Process( SoftClip( in[0][i] * ( 1.f + currentVenomValue ) ) ) ) );
		out[1][i] = 0.f;
		out[2][i] = 0.f;
		out[3][i] = 0.f;
	}
}
void initializeComb(){
	int combBufferSize = COMB_BUFFER_SIZE;
	for( int i = 0; i < combBufferSize; i++ ) combBuffer[ i ] = 0.f;
	comb.Init( SAMPLE_RATE, combBuffer, combBufferSize );
}
void initializeComp(){
	comp.Init( SAMPLE_RATE );	
	updateCurrentCompAttack();
	updateCurrentCompMakeup();
	updateCurrentCompRelease();
	updateCurrentCompThreshold();
}
void initializeKnobs(){
	growlKnob.Init( hw.controls[0], 20.f, 40000.f, Parameter::LOGARITHMIC );
	howlKnob.Init( hw.controls[2], 0.f, 6000.f, Parameter::LOGARITHMIC );
	resKnob.Init( hw.controls[1], 0.f, 0.9f, Parameter::LINEAR );
	fdbkKnob.Init( hw.controls[3], 0.f, 0.99f, Parameter::EXPONENTIAL );
}
void updateOledTopLeft(){
	FixedCapStr<8> str("");
	switch( currentMenu ){
		case MENU_TOP:
			str = "TKSHKA  ";
			break;
		case MENU_VENOM:
			str = "VENOM   ";
			break;
		case MENU_COMP:
			if( isCompSubmenu ){
				switch( currentCompSetting ){
					case COMP_ATTACK:
						str = "ATTACK   ";
						break;
					case COMP_MAKEUP:
						str = "MAKEUP  ";
						break;
					case COMP_RATIO:
						str = "RATIO   ";
						break;
					case COMP_RELEASE:
						str = "RELEASE ";
						break;
					case COMP_THRESHOLD:
						str = "THRSHOLD";
						break;
					default: break;
				}
			} else str = "COMP    ";
			break;
		case MENU_FILTER_ORDER:
			str = "FILTER  ";
			break;
		default:
			str = "        ";
			break;
	}

	hw.display.SetCursor( 0, 0 );
	hw.display.WriteString( str, Font_7x10, true );
}
void updateOledTopRight(){
	FixedCapStr<8> str("");
	switch( currentMenu ){
		case MENU_TOP:
			str = "   MENU:";
			break;
		case MENU_VENOM:
			str = " 0-8.0";
			break;
		case MENU_COMP:
			if( isCompSubmenu ){				
				switch( currentCompSetting ){
					case COMP_ATTACK:
						str = "0.01-10s";
						break;
					case COMP_MAKEUP:
						str = "  0-80db";
						break;
					case COMP_RATIO:
						str = "    1-40";
						break;
					case COMP_RELEASE:
						str = "0.01-10s";
						break;
					case COMP_THRESHOLD:
						str = "  0-80db";
						break;
					default:
						str = "        ";
						break;
				}
			} else str = "        ";
			break;
		default:
			str = "        ";
			break;
	}
	hw.display.SetCursor( 64, 0 );
	hw.display.WriteString( str, Font_7x10, true );
}
void updateOledBottomRight(){
	FixedCapStr<8> str("");
	switch( currentMenu ){
		case MENU_TOP:
			switch( currentTopMenuSetting ){
				case MENU_VENOM:
					str = "   VENOM";
					break;
				case MENU_FILTER_ORDER:
					str = "  FILTER";
					break;
				case MENU_COMP:
					str = "    COMP";
					break;
				default: 
					str = "        ";
					break;
			}
			break;
		case MENU_VENOM:
			str.Clear();
			str.Append( "   " );
			str.AppendFloat(currentVenomValue, 2, false, true );
			break;
		case MENU_FILTER_ORDER:
			switch( currentFilterOrder ){
				case FILTER_ORDER_LADDER_FIRST:
					str = "LAD->LPF";
					break;
				case FILTER_ORDER_COMB_FIRST:
					str = "LFP->LAD";
					break;
				default: break;
			}
			break;
		case MENU_COMP:
			if( isCompSubmenu ){
				switch( currentCompSetting ){
					case COMP_ATTACK:
						str.Clear();
						str.AppendFloat( comp.GetAttack() );
						break;
					case COMP_MAKEUP:
						str.Clear();
						str.AppendFloat( comp.GetMakeup() );
						break;
					case COMP_RATIO:
						str.Clear();
						str.AppendFloat( comp.GetRatio() );
						break;
					case COMP_RELEASE:
						str.Clear();
						str.AppendFloat( comp.GetRelease() );
						break;
					case COMP_THRESHOLD:
						str.Clear();
						str.AppendFloat( comp.GetThreshold() );
						break;
					default:
						str = "        ";
						break;
				}
			} else {
				switch( currentCompSetting ){
					case COMP_ATTACK:
						str = "  ATTACK";
						break;
					case COMP_MAKEUP:
						str = "  MAKEUP";
						break;
					case COMP_RATIO:
						str = "   RATIO";
						break;
					case COMP_RELEASE:
						str = " RELEASE";
						break;
					case COMP_THRESHOLD:
						str = "THRSHOLD";
						break;
					default: // 'BACK' BUTTON
						str = "      <-";
						break;
				}
			}
			break;
		default:
			str = "        ";
			break;
	}
	hw.display.SetCursor( 64, 32 );
	hw.display.WriteString( str, Font_7x10, true );
}
void updateOledBottomLeft(){
	FixedCapStr<8> str("");
	switch( currentMenu ){
		case MENU_TOP:
			str = "FILTER  ";
			break;
		case MENU_FILTER_ORDER:
			str = "ORDER   ";
			break;
		case MENU_COMP:
			if( isCompSubmenu ){
				switch( currentCompSetting ){
					case COMP_ATTACK:
						str.Clear();
						str.AppendInt( currentCompAttackIncrement );
						break;
					case COMP_MAKEUP:
						str.Clear();
						str.AppendInt( currentCompMakeupIncrement );
						break;
					case COMP_RATIO:
						str.Clear();
						str.AppendInt( currentCompRatioIncrement );
						break;
					case COMP_RELEASE:
						str.Clear();
						str.AppendInt( currentCompReleaseIncrement );
						break;
					case COMP_THRESHOLD:
						str.Clear();
						str.AppendInt( currentCompThresholdIncrement );
						break;
					default:
						str = "        ";
						break;
				}
			} else {
				str = "SETTINGS";
			}
			break;
		default:
			str = "        ";
			break;
	}
	hw.display.SetCursor( 0, 32 );
	hw.display.WriteString( str, Font_7x10, true );
}
void updateOled(){
	if( updateDisplay ){
		// oled is 128x64 pixels
		hw.display.Fill( false );
		updateOledTopLeft();
		updateOledTopRight();
		updateOledBottomLeft();
		updateOledBottomRight();
		hw.display.Update();
		updateDisplay = false;
	}
}
int main( void ){
	hw.Init();
	hw.SetAudioBlockSize( 4 ); // number of samples handled per callback
	hw.SetAudioSampleRate( SaiHandle::Config::SampleRate::SAI_48KHZ );
	ladder.Init( SAMPLE_RATE );
	initializeComb();
	initializeKnobs();
	initializeComp();
	updateVenomValue();
	hw.StartAdc();
	hw.StartAudio( AudioCallback );
	while( true ){
		hw.ProcessAllControls();
		ladder.SetFreq( growlKnob.Process() );
		ladder.SetRes( resKnob.Process() );
		comb.SetFreq( howlKnob.Process () );
		comb.SetRevTime( fdbkKnob.Process() );
		handleEncoder();
		updateOled();
	}
}