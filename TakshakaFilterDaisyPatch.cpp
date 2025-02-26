#include "daisy_patch.h"
#include "daisysp.h"
#include "Filters/moogladder.h"
#include "Filters/comb.h"
#include "compressor.h"

#define SAMPLE_RATE 48000.f
#define COMB_BUFFER_SIZE 9600
#define MAX_COMP_INCREMENT 20
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
bool isCompSubmenu = false;
float combFilterBuffer[ 9600 ],
	currentVenomValue = 0.f,
	currentCompAttack = 0.01f,
	currentCompMakeup = 1.f,
	currentCompRatio = 1.f,
	currentCompRelease = 0.1f,
	currentCompThreshold = -1.f,
	compAttackMin = 0.001f, // 0.001 -> 10
	compMakeupMin = 0.f, // 0.0 -> 80
	compRatioMin = 1.f, // 1.0 -> 40
	compReleaseMin = 0.001f, // 0.001 -> 10
	compThresholdMin = 0.f, // 0.0 -> -80
	compAttackMax = 10.f,
	compMakeupMax = 80.f,
	compRatioMax = 40.f,
	compReleaseMax = 10.f,
	compThresholdMax = -80.f;
int currentMenu = MENU_TOP,
	currentCompSetting = COMP_ATTACK,
	currentFilterOrder = FILTER_ORDER_LADDER_FIRST,
	currentMenuMode = MENU_MODE_SELECT,
	compAttackCurve = CURVE_LOGARITHMIC,
	compMakeupCurve = CURVE_LOGARITHMIC,
	compRatioCurve = CURVE_LINEAR,
	compReleaseCurve = CURVE_LOGARITHMIC,
	compThresholdCurve = CURVE_LOGARITHMIC,
	currentCompAttackIncrement = 1,
	currentCompMakeupIncrement = 1,
	currentCompRatioIncrement = 1,
	currentCompReleaseIncrement = 1,
	currentCompThresholdIncrement = 1,
	currentCompSubMenu = COMP_ATTACK;
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
void handleCompMenuEncoderUp() {
	if( currentMenuMode == MENU_MODE_SELECT ){		
		currentCompSetting++;
		if( currentCompSetting >= COMP_SETTINGS_COUNT ) 
			currentCompSetting = COMP_SETTINGS_COUNT - 1;
	}
	if( currentMenuMode == MENU_MODE_EDIT ){		
		switch( currentCompSetting ){
			case COMP_ATTACK:
				currentCompAttackIncrement++;
				if( currentCompAttackIncrement > MAX_COMP_INCREMENT ) currentCompAttackIncrement = MAX_COMP_INCREMENT;
				currentCompAttack = processCurve( 
					(float) currentCompAttackIncrement / (float) MAX_COMP_INCREMENT,
					compAttackMin,
					compAttackMax,
					CURVE_EXPONENTIAL
				);
				break;
			case COMP_MAKEUP:
				currentCompMakeupIncrement++;
				if( currentCompMakeupIncrement > MAX_COMP_INCREMENT ) currentCompMakeupIncrement = MAX_COMP_INCREMENT;
				currentCompMakeup = processCurve( 
					(float) currentCompMakeupIncrement / (float) MAX_COMP_INCREMENT,
					compMakeupMin,
					compMakeupMax,
					CURVE_EXPONENTIAL
				);
				break;
			case COMP_RATIO:			
				currentCompRatioIncrement++;
				if( currentCompRatioIncrement > MAX_COMP_INCREMENT ) currentCompRatioIncrement = MAX_COMP_INCREMENT;
				currentCompRatio = processCurve( 
					(float) currentCompRatioIncrement / (float) MAX_COMP_INCREMENT,
					compRatioMin,
					compRatioMax,
					CURVE_LINEAR
				);
				break;
			case COMP_RELEASE:			
				currentCompReleaseIncrement++;
				if( currentCompReleaseIncrement > MAX_COMP_INCREMENT ) currentCompReleaseIncrement = MAX_COMP_INCREMENT;
				currentCompRelease = processCurve( 
					(float) currentCompReleaseIncrement / (float) MAX_COMP_INCREMENT,
					compReleaseMin,
					compReleaseMax,
					CURVE_EXPONENTIAL
				);
				break;
			case COMP_THRESHOLD:
				currentCompThresholdIncrement++;
				if( currentCompThresholdIncrement > MAX_COMP_INCREMENT ) currentCompThresholdIncrement = MAX_COMP_INCREMENT;
				currentCompThreshold = processCurve( 
					(float) currentCompThresholdIncrement / (float) MAX_COMP_INCREMENT,
					compThresholdMin,
					compThresholdMax,
					CURVE_EXPONENTIAL
				);
				break;
			default: break;
		}
	}
}
void handleCompMenuEncoderDown() {
	if( currentMenuMode == MENU_MODE_SELECT ){		
		currentCompSetting--;
		if( currentCompSetting < 1 ) currentCompSetting = 1;
	}
	if( currentMenuMode == MENU_MODE_EDIT ){		
		switch( currentCompSetting ){
			case COMP_ATTACK:
				currentCompAttackIncrement--;
				if( currentCompAttackIncrement < 1 ) currentCompAttackIncrement = 1;
				currentCompAttack = processCurve( 
					(float) currentCompAttackIncrement / (float) MAX_COMP_INCREMENT,
					compAttackMin,
					compAttackMax,
					CURVE_EXPONENTIAL
				);
				break;
			case COMP_MAKEUP:
				currentCompMakeupIncrement--;
				if( currentCompMakeupIncrement < 1 ) currentCompMakeupIncrement = 1;
				currentCompMakeup = processCurve( 
					(float) currentCompMakeupIncrement / (float) MAX_COMP_INCREMENT,
					compMakeupMin,
					compMakeupMax,
					CURVE_EXPONENTIAL
				);
				break;
			case COMP_RATIO:			
				currentCompRatioIncrement--;
				if( currentCompRatioIncrement < 1 ) currentCompRatioIncrement = 1;
				currentCompRatio = processCurve( 
					(float) currentCompRatioIncrement / (float) MAX_COMP_INCREMENT,
					compRatioMin,
					compRatioMax,
					CURVE_LINEAR
				);
				break;
			case COMP_RELEASE:			
				currentCompReleaseIncrement--;
				if( currentCompReleaseIncrement < 1 ) currentCompReleaseIncrement = 1;
				currentCompRelease = processCurve( 
					(float) currentCompReleaseIncrement / (float) MAX_COMP_INCREMENT,
					compReleaseMin,
					compReleaseMax,
					CURVE_EXPONENTIAL
				);
				break;
			case COMP_THRESHOLD:
				currentCompThresholdIncrement--;
				if( currentCompThresholdIncrement < 1 ) currentCompThresholdIncrement = 1;
				currentCompThreshold = processCurve( 
					(float) currentCompThresholdIncrement / (float) MAX_COMP_INCREMENT,
					compThresholdMin,
					compThresholdMax,
					CURVE_EXPONENTIAL
				);
				break;
			default: break;
		}
	}
}
void encoderUp(){
	switch( currentMenu ){
		case MENU_VENOM:			
			currentVenomValue = currentVenomValue + 0.05f;
			if( currentVenomValue > 8.f ) currentVenomValue = 8.f;
			break;
		case MENU_FILTER_ORDER:
			currentFilterOrder++;
			if( currentFilterOrder >= FILTER_ORDERS_COUNT ) currentFilterOrder = FILTER_ORDER_COMB_FIRST;
			break;
		case MENU_COMP:
			handleCompMenuEncoderUp();
	}
}
void encoderDown(){
	switch( currentMenu ){
		case MENU_VENOM:
			currentVenomValue = currentVenomValue - 0.05f;
			if( currentVenomValue < 0.f ) currentVenomValue = 0.f;
			break;
		case MENU_FILTER_ORDER:
			currentFilterOrder--;
			if( currentFilterOrder < FILTER_ORDER_LADDER_FIRST ) currentFilterOrder = FILTER_ORDER_LADDER_FIRST;
			break;
		case MENU_COMP:
			handleCompMenuEncoderDown();
	}
}
void encoderClick(){
	switch( currentMenu ){
		// case MENU_TOP:
		case MENU_VENOM:
			currentMenu = MENU_TOP;
			break;
		case MENU_FILTER_ORDER:
			currentMenu = MENU_TOP;
		case MENU_COMP:
			switch( currentCompSubMenu ){
				// case:
			}
		default: break;
	}
}
void handleEncoder(){
	int increment = hw.encoder.Increment();
	if( increment == 1 ) encoderUp();
	if ( increment == -1 ) encoderDown();
	if( hw.encoder.RisingEdge() ) encoderClick();
}
void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size){
	for (size_t i = 0; i < size; i++){
		if( currentFilterOrder == FILTER_ORDER_COMB_FIRST )
			out[0][i] = ladder.Process( comb.Process( SoftClip( in[0][i] * ( 1.f + currentVenomValue ) ) ) );		
		if( currentFilterOrder == FILTER_ORDER_LADDER_FIRST )
			out[0][i] = comb.Process( ladder.Process( SoftClip( in[0][i] * ( 1.f + currentVenomValue ) ) ) );
		// todo: normalize volume level after the ladder filter, it can be very low at higher resonance settings.
		out[1][i] = 0.f;
		out[2][i] = 0.f;
		out[3][i] = 0.f;
	}
}
std::string leftPad( std::string in, int length ){
	std::string output;
	if( in.length() > length ) {
		in.resize( length );
		output = in;
	}
	if( (int) in.length() < length ){
		output = "";
		int toAdd = length - in.length();
		for( int i = 0; i < toAdd; i++ ){
			output += " ";
		}
		output += in;
	}
	return output;
}
std::string rightPad( std::string in, int length ){
	in.resize( length );
	return in;
}
void updateOled(){



// OLED IS SPLIT INTO 4 QUADRANTS:
// WE HANDLE THIS IN 4 STEPS BELOW
// EACH QUADRANT IS 8 CHARS
// oled is 128x64 pixels


std::string str;
char* cstr = &str[0];
	
	
// HANDLE BOTTOM LEFT QUADRANT
	
	switch( currentMenu ){
		case MENU_TOP:
			str = "FILTER  ";
			break;
		case MENU_COMP:
			if( isCompSubmenu ){
				switch( currentCompSubMenu ){
					case COMP_ATTACK:
						sprintf( cstr, "   %2s   ", currentCompAttackIncrement );
						break;
					case COMP_MAKEUP:
						sprintf( cstr, "   %2s   ", currentCompMakeupIncrement );
						break;
					case COMP_RATIO:
						sprintf( cstr, "   %2s   ", currentCompRatioIncrement );
						break;
					case COMP_RELEASE:
						sprintf( cstr, "   %2s   ", currentCompReleaseIncrement );
						break;
					case COMP_THRESHOLD:
						sprintf( cstr, "   %2s   ", currentCompThresholdIncrement );
						break;
					default:
						cstr = "        ";
						break;
				}
			}
			break;
		default: 
			str = "        ";
			break;
	}
	hw.display.SetCursor( 0, 32 );
	hw.display.WriteString( cstr, Font_7x10, true );

	// HANDLE THE TOP RIGHT QUADRANT (shows the current setting's range)
	switch( currentMenu ){
		case MENU_VENOM:
			str = "  1 - 20";
			break;
		case MENU_COMP:
			switch( currentCompSubMenu ){
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
			break;
		default: 
			str = "        ";
			break;
	}
	hw.display.SetCursor( 64, 0 );
	hw.display.WriteString( cstr, Font_7x10, true );

	// HANDLE THE BOTTOM RIGHT QUADRANT
	switch( currentMenu ){
		case MENU_VENOM:
			sprintf( cstr, "      %02s", currentVenomValue );
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
			switch( currentCompSubMenu ){
				case COMP_ATTACK:
				sprintf( cstr, "      %02s", currentCompAttack );
				break;
			case COMP_MAKEUP:
				sprintf( cstr, "      %02s", currentCompMakeup );
				break;
			case COMP_RATIO:
				sprintf( cstr, "      %02s", currentCompRatio );
				break;
			case COMP_RELEASE:
			sprintf( cstr, "      %02s", currentCompRelease );
				break;
			case COMP_THRESHOLD:
			sprintf( cstr, "      %02s", currentCompThreshold );
				break;
			default:
				str = "        ";
				break;
			}
			break;
		default: 
			str = "        ";
			break;
	}
	hw.display.SetCursor( 64, 32 );
	hw.display.WriteString( cstr, Font_7x10, true );

	hw.display.Update();
}
void initializeComb(){
	int combBufferSize = COMB_BUFFER_SIZE;
	float combBuffer[combBufferSize];
	for( int i = 0; i < combBufferSize; i++ ) combFilterBuffer[ i ] = 0.f;		
	comb.Init( SAMPLE_RATE, combBuffer, combBufferSize );
}
void initializeComp(){
	comp.Init( SAMPLE_RATE );
	comp.SetAttack( processCurve( currentCompAttackIncrement, compAttackMin, compAttackMax, CURVE_EXPONENTIAL ) );
	comp.SetMakeup( processCurve( currentCompMakeupIncrement, compMakeupMin, compMakeupMax, CURVE_EXPONENTIAL ) );
	comp.SetRatio( processCurve( currentCompRatioIncrement, compRatioMin, compRatioMax, CURVE_LINEAR ) );
	comp.SetRelease(  processCurve( currentCompReleaseIncrement, compReleaseMin, compReleaseMax, CURVE_EXPONENTIAL ) );
	comp.SetThreshold(  processCurve( currentCompThresholdIncrement, compThresholdMin, compThresholdMax, CURVE_EXPONENTIAL ) );
}
void initializeKnobs(){
	growlKnob.Init( hw.controls[0], 20.f, 20000.f, Parameter::LOGARITHMIC );
	howlKnob.Init( hw.controls[2], 0.f, 6000.f, Parameter::LOGARITHMIC );
	resKnob.Init( hw.controls[1], 0.f, 0.9f, Parameter::LINEAR );
	fdbkKnob.Init( hw.controls[3], 0.f, 0.99f, Parameter::EXPONENTIAL );
}
int main( void ){
	hw.Init();
	hw.SetAudioBlockSize( 4 ); // number of samples handled per callback
	hw.SetAudioSampleRate( SaiHandle::Config::SampleRate::SAI_48KHZ );
	ladder.Init( SAMPLE_RATE );
	initializeComb();
	initializeComp();
	initializeKnobs();
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
