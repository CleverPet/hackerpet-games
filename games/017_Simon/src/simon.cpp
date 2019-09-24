/**
  Simon
  =========================


  Authors: CleverPet Inc.
           Jelmer Tiete <jelmer@tiete.be>

  Copyright 2019
  Licensed under the AGPL 3.0
*/

#include <hackerpet.h>
#include <algorithm>


/* Yield Sleep milliseconds with timeout
 *
 * Waits for the specified number of milliseconds, yielding while waiting.
 * Uses millis() function, which overflows and returns to zero every ~49 days
 */
/* Yield Wait For with timeout
 *
 * Waits for a condition while yielding whenever the condition is not true.
 * Passes the ret parameter whenever yielding.
 */
#define yield_wait_for_with_timeout(condition, timeout_time_in_milliseconds, ret)\
  do {                                                                         \
    static unsigned long t1 = 0;                                               \
    t1 = millis();                                                             \
    while (!(condition) && (millis() - t1)                                     \
           < timeout_time_in_milliseconds) {                                   \
      yield(ret);                                                              \
    }                                                                          \
  } while (0)


// Set this to the name of your player (dog, cat, etc.)
const char PlayerName[] = "Pet, Clever";

/**
 * Challenge settings
 * -------------
 *
 * These constants (capitalized CamelCase) and variables (camelCase) define the
 * gameplay
 */
int currentLevel = 1; // starting level
const int HISTORY_LENGTH=      5;   // Number of past interactions to look at for performance
const int ENOUGH_SUCCESSES=    4;   // if successes >= ENOUGH_SUCCESSES level-up
const int TOO_MANY_MISSES=     4;   // if num misses >= TOO_MANY_MISSES level-down
const int REINFORCE_RATIO =      100; // the foodtreat reinforcement ratio [0-100] 100:always foodtreat
// LED colors and intensities
const int CUE_LIGHT_PRESENT_INTENSITY_RED = 99; // [0-99] // cue / status light is yellow in present phase
const int CUE_LIGHT_PRESENT_INTENSITY_GREEN = 99; // [0-99]
const int CUE_LIGHT_PRESENT_INTENSITY_BLUE = 0; // [0-99]
const int CUE_LIGHT_RESPONSE_INTENSITY_RED = 5; // [0-99] // cue / status light is white in response phase
const int CUE_LIGHT_RESPONSE_INTENSITY_GREEN = 5; // [0-99]
const int CUE_LIGHT_RESPONSE_INTENSITY_BLUE = 5; // [0-99]
const int SLEW = 0; // slew for all lights [0-99]
const int TARGET_PRESENT_INTENSITY_RED = 80; // [0-99]
const int TARGET_PRESENT_INTENSITY_GREEN = 80; // [0-99]
const int TARGET_PRESENT_INTENSITY_BLUE = 80; // [0-99]
const int TARGET_RESPONSE_INTENSITY_RED = 80; // [0-99]
const int TARGET_RESPONSE_INTENSITY_GREEN = 80; // [0-99]
const int TARGET_RESPONSE_INTENSITY_BLUE = 80; // [0-99]
const unsigned long FOODTREAT_DURATION = 4000; // (ms) how long to present foodtreat
const unsigned long TIMEOUT_INTERACTIONS_MS = 5000; // (ms) how long to wait until restarting the
                                                    // interaction
const unsigned long INTER_GAME_DELAY = 5000; // timeout inbetween games on miss
const double HINT_INTENSITY_MULTIPL[] = {1.00,0.30,0,0,1.00,1.00,1.00,1.00,1.00,
							0.80,0.70,0.50,0.40,0.30,0.20,0.15,0.10,0.05,0.02,0};
const int END_ON_MISS_CHANCE_1[] = {100,0,25,100};
const int END_ON_MISS_CHANCE_2[] = {0,15,20,25,30,35,40,45,50,55,60,65,70,75,85,100};
const int END_ON_MISS_CHANCE_3[] = {0,40,50,55,60,65,70,75,80,85,90,95,100,100,100,100};

/**
 * Global variables and constants
 * ------------------------------
 */
const unsigned long SOUND_AUDIO_POSITIVE_DELAY = 350; // (ms) delay for reward sound
const unsigned long SOUND_AUDIO_NEGATIVE_DELAY = 350;
const unsigned long SOUND_TOUCHPAD_DELAY = 300; // (ms) delay for touchpad sound
const unsigned long SOUND_DO_DELAY = 150; // (ms) delay for reward sound

bool performance[HISTORY_LENGTH] = {0}; // store the progress in this challenge
unsigned char perfPos = 0; // to keep our position in the performance array
unsigned char perfDepth = 0; // to keep the size of the number of perf numbers to consider

// Use primary serial over USB interface for logging output (9600)
// Choose logging level here (ERROR, WARN, INFO)
SerialLogHandler logHandler(LOG_LEVEL_INFO, { // Logging level for all messages
    { "app.hackerpet", LOG_LEVEL_ERROR }, // Logging level for library messages
    { "app", LOG_LEVEL_INFO } // Logging level for application messages
});

// access to hub functionality (lights, foodtreats, etc.)
HubInterface hub;

// enables simultaneous execution of application and system thread
SYSTEM_THREAD(ENABLED);

/**
 * Helper functions
 * ----------------
 */

/// return the number of successes in performance history for current level
unsigned int countSuccesses(){
    unsigned int total = 0;
    for (unsigned char i = 0; i <= perfDepth-1 ; i++)
        if(performance[i]==1)
            total++;
    return total;
}

/// return the number of misses in performance history for current level
unsigned int countMisses(){
    unsigned int total = 0;
    for (unsigned char i = 0; i <= perfDepth-1 ; i++)
        if(performance[i]==0)
            total++;
    return total;
}

/// reset performance history to 0
void resetPerformanceHistory(){
    for (unsigned char i = 0; i < HISTORY_LENGTH ; i++)
        performance[i] = 0;
    perfPos = 0;
    perfDepth = 0;
}

/// add a interaction result to the performance history
void addResultToPerformanceHistory(bool entry){
        // Log.info("Adding %u", entry);
    performance[perfPos] = entry;
    perfPos++;
    if (perfDepth < HISTORY_LENGTH)
        perfDepth++;
    if (perfPos > (HISTORY_LENGTH - 1)){ // make our performance array circular
        perfPos = 0;
    }
    // Log.info("perfPos %u, perfDepth %u", perfPos, perfDepth);
    Log.info("New successes: %u, misses: %u", countSuccesses(),
             countMisses());

}

/// print the performance history for debugging
void printPerformanceArray(){
    Serial.printf("performance: {");
    for (unsigned char i = 0; i < perfDepth ; i++){
        Serial.printf("%u",performance[i]);
        if ((i+1) == perfPos)
            Serial.printf("|");
    }
    Serial.printf("}\n");
}
/// converts a bitfield of pressed touchpads to letters
/// multiple consecutive touches are possible and will be reported L -> M - > R
/// @returns String
String convertBitfieldToLetter(unsigned char pad){
  String letters = "";
  if (pad & hub.BUTTON_LEFT)
    letters += 'L';
  if (pad & hub.BUTTON_MIDDLE)
    letters += 'M';
  if (pad & hub.BUTTON_RIGHT)
    letters += 'R';
  return letters;
}

/// converts requested touchpad bitfield and pressed touchpad bitfield to a
/// letter. Requested bitfield should have only one bit set. Touched pad
/// bitfield can have multiple bits set. If correct, the correct pad will be
/// returned, if wrong only the wrong pad will be returned, if multiple wrong
/// pads pressed, only one wrong pad will be returned in order L -> M -> R
/// @returns String
String convertBitfieldToSingleLetter(unsigned char targetPad, unsigned char pad){
  if ((targetPad & (targetPad-1)) != 0) // error targetPad has multiple pads set
    return "X";

  String letter = "";
  if (targetPad == pad){ // did we get it right?
    letter += convertBitfieldToLetter(targetPad); // report right pad
  }
  else { // we have a wrong pad touched or multiple pads touched (of which one is wrong)
    unsigned char maskedPressed = ~targetPad & pad; // mask out the correct pad
    // check if multiple pads touched (except for correct one)
    if ((maskedPressed & (maskedPressed-1)) != 0)
    {
      // multiple wrong pads touched
      //just pick one to report, L -> M -> R
      if (maskedPressed & hub.BUTTON_LEFT)
        letter += 'L';
      else if (maskedPressed & hub.BUTTON_MIDDLE)
        letter += 'M';
      else if (maskedPressed & hub.BUTTON_RIGHT)
        letter += 'R';
    } else {
      //only one wrong pad touched
      letter += convertBitfieldToLetter(maskedPressed);
    }
  }
  return letter;
}

int buttonToAudio(unsigned char button){
	switch(button) {
    case hub.BUTTON_LEFT : return hub.AUDIO_L;
             break;
    case hub.BUTTON_MIDDLE : return hub.AUDIO_M;
             break;
    case hub.BUTTON_RIGHT : return hub.AUDIO_R;
             break;
    default: return 0;
	}
}

/// The actual LearningLongerSequences function. This function needs to be called in a loop.
bool playSimon(){
  yield_begin();

  static const int SEQUENCE_LENGTHMAX = 20;
  static const int LOG_LENGTH_MAX = 100;
  static unsigned long timestampBefore, timestampTouchpad, gameStartTime, activityDuration = 0;
  static unsigned long touchLogTimes[SEQUENCE_LENGTHMAX] = {};
  static unsigned char foodtreatState = 99;
  static unsigned char touchpads[3]={hub.BUTTON_LEFT,
                                   hub.BUTTON_MIDDLE,
                                   hub.BUTTON_RIGHT};
  static unsigned char sequence_pos = 0;
  static double hintIntensityMultipl = 0;
  static unsigned char touchpad_sequence[SEQUENCE_LENGTHMAX]={};
  static unsigned char pressed[SEQUENCE_LENGTHMAX] = {};
  static unsigned char touchLog[LOG_LENGTH_MAX] = {};  // could end up longer than sequence length, should use vector
  static int sequenceLength = 0; // sequence length (gets calculated)
  static int touchLogIndex = 0;
  static int presentMisses = 0; // logging error touches during present phase
  static int responseMisses = 0; // logging error touches during response phase
  static bool accurate = false;
  static bool timeout = false;
  static bool foodtreatPresented = false; // store if foodtreat was presented
  static bool foodtreatWasEaten = false; // store if foodtreat was eaten in last interaction
  static bool retrySequence = false; // do not re-initialize
  static bool dodoSoundPlayed = false;
  // Static variable and constants are only initialized once, and need to be re-initialized
  // on subsequent calls
  sequenceLength = 0;
  timestampBefore = 0;
  timestampTouchpad = 0;
  gameStartTime = 0;
  activityDuration = 0;
  foodtreatState = 99;
  touchpads[0]=hub.BUTTON_LEFT;
  touchpads[1]=hub.BUTTON_MIDDLE;
  touchpads[2]=hub.BUTTON_RIGHT;
  sequence_pos = 0;
  // reset pressed touchpads
  fill(pressed, pressed+SEQUENCE_LENGTHMAX, 0);
  // fill(respTimes, respTimes+ sizeof( respTimes ), 0); //DONT DO THIS
  for (int i = 0; i<SEQUENCE_LENGTHMAX; i++){
    touchLog[i]=0;
  }
  for (int i = 0; i<SEQUENCE_LENGTHMAX; i++){
    touchLogTimes[i]=0;
  }
  accurate = false;
  timeout = false;
  hintIntensityMultipl = 0;
  foodtreatPresented = false; // store if foodtreat was presented in last interaction
  foodtreatWasEaten = false; // store if foodtreat was eaten in last interaction
  dodoSoundPlayed = false;
  touchLogIndex = 0;
  presentMisses = 0;//store if touchpad was touched during presentation phase
  responseMisses = 0;

  Log.info("-------------------------------------------");
  Log.info("Starting new \"Simon\" challenge");

  // before starting interaction, wait until:
  //  1. device layer is ready (in a good state)
  //  2. foodmachine is "idle", meaning it is not spinning or dispensing foodtreat
  //      and tray is retracted (see FOODMACHINE_... constants)
  //  3. no button is currently pressed
  yield_wait_for((hub.IsReady()
                  && hub.FoodmachineState() == hub.FOODMACHINE_IDLE
                  && not hub.AnyButtonPressed()), false);

  // DI reset occurs if, for example, device  layer detects that touchpads need re-calibration
  hub.SetDIResetLock(true);

  gameStartTime = Time.now();

  //calculate sequenceLength
  if (currentLevel < 5)
  	sequenceLength = 1;
  else
  	sequenceLength = ((currentLevel-5)/16)+2; // see game-logic chart

	if(!retrySequence){
    // fill touchpad_sequence
    for (int i = 0; i < sequenceLength; ++i)
    {
        random_shuffle(&touchpads[0], &touchpads[3]);
        touchpad_sequence[i] = touchpads[0];
    }
	} else {
		Log.info("Doing a retry game");
		//reset retrySequence
		retrySequence = false;
	}

  Log.info("Current level: %u, sequence length: %u, successes: %u, misses: %u",
  currentLevel, sequenceLength, countSuccesses(), countMisses());
  // log sequence
  
  // String seq = "Sequence: ";
  // for (int i = 0; i < sequenceLength; ++i){
  //     seq += convertBitfieldToLetter(touchpad_sequence[i]);
  // }
  // Log.info(seq);

//------------------------------------------------------------------------------
    // PRESENTATION PHASE

  // Record start timestamp for performance logging
  timestampBefore = millis();

  // turn off the button sounds
  hub.SetButtonAudioEnabled(0);

 	// illuminate cue light Yellow
	hub.SetLightsRGB(
		hub.LIGHT_CUE,
		CUE_LIGHT_PRESENT_INTENSITY_RED,
		CUE_LIGHT_PRESENT_INTENSITY_GREEN,
		CUE_LIGHT_PRESENT_INTENSITY_BLUE,
		SLEW);
	
	// play DO sound
	hub.PlayAudio(hub.AUDIO_DO, 90);
    // give the Hub a moment to finish playing the sound and detect touches
    // yield_sleep_ms(SOUND_DO_DELAY+500, false);
    // extra delay between do sound and presentation of sequence and detect touches
    // yield_sleep_ms(500, false);
  yield_wait_for_with_timeout(hub.AnyButtonPressed(), SOUND_DO_DELAY+500,false);
  if(!hub.AnyButtonPressed()){
    // illuminate sequence
    for (sequence_pos = 0; sequence_pos < sequenceLength; ++sequence_pos) {
    	hub.SetLightsRGB(
        touchpad_sequence[sequence_pos],
        TARGET_PRESENT_INTENSITY_RED,
        TARGET_PRESENT_INTENSITY_GREEN,
        TARGET_PRESENT_INTENSITY_BLUE,
        SLEW);
  		// play touchpad sound
  		hub.PlayAudio(buttonToAudio(touchpad_sequence[sequence_pos]), 60);
    	// give the Hub a moment to finish playing the sound and detect touches
    	yield_wait_for_with_timeout(hub.AnyButtonPressed(), SOUND_TOUCHPAD_DELAY+200,false);
    	if(hub.AnyButtonPressed()){break;}
    	
    	// turn off touchpad light
    	hub.SetLights(touchpad_sequence[sequence_pos],0,0,SLEW);
    }
  }

  // yield_sleep_ms(1300, false);
  // wait time before response phase and detect touches
  yield_wait_for_with_timeout(hub.AnyButtonPressed(), 50,false);
  if(hub.AnyButtonPressed()){
    touchLog[touchLogIndex] = hub.AnyButtonPressed();
    touchLogTimes[touchLogIndex] = millis() - timestampBefore;
    touchLogIndex++;
    presentMisses++;}

  // turn off touchpad lights
  hub.SetLights(hub.LIGHT_BTNS,0,0,SLEW);

  // turn off cue light
	// hub.SetLights(hub.LIGHT_CUE, 0, 0, SLEW);


//------------------------------------------------------------------------------
    // RESPONSE PHASE

  // wait until: no button is currently pressed
  yield_wait_for((!hub.AnyButtonPressed()), false);

   	// illuminate cue light White
	hub.SetLightsRGB(
		hub.LIGHT_CUE,
		CUE_LIGHT_RESPONSE_INTENSITY_RED,
		CUE_LIGHT_RESPONSE_INTENSITY_GREEN,
		CUE_LIGHT_RESPONSE_INTENSITY_BLUE,
		SLEW);

	if(!presentMisses){
		// delay after turning cue light to white
	    yield_sleep_ms(400, false);

    // start response detection
    for (sequence_pos = 0; sequence_pos < sequenceLength; ++sequence_pos)
    {
    	// wait until: no button is currently pressed
    	yield_wait_for((!hub.AnyButtonPressed()), false);

    	// turn on response hint, see game logic table for intensity calculation
      if (currentLevel < 5){
       hintIntensityMultipl = HINT_INTENSITY_MULTIPL[currentLevel-1];
      } else {
       hintIntensityMultipl = HINT_INTENSITY_MULTIPL[(((currentLevel-5) % 16 ) + 5 ) - 1];
      }

      Log.info(String(currentLevel));
      Log.info(String(sequence_pos));
      Log.info(String(hintIntensityMultipl));

      hub.SetLightsRGB(
        touchpad_sequence[sequence_pos],
        (TARGET_RESPONSE_INTENSITY_RED * hintIntensityMultipl),
        (TARGET_RESPONSE_INTENSITY_GREEN * hintIntensityMultipl),
        (TARGET_RESPONSE_INTENSITY_BLUE * hintIntensityMultipl),
        SLEW);

      // on first light in sequence, play DODO sound
      if (sequence_pos == 0 && !dodoSoundPlayed){
        // play DODO sound
        hub.PlayAudio(hub.AUDIO_DO, 90);
        yield_sleep_ms(SOUND_DO_DELAY, false);
        hub.PlayAudio(hub.AUDIO_DO, 90);
        yield_sleep_ms(SOUND_DO_DELAY, false);
        dodoSoundPlayed = true;
      }


    	// turn on the button sounds
    	hub.SetButtonAudioEnabled(1);

    	timestampTouchpad = millis();
    	do
    	{
    	    // detect any buttons currently pressed
    	    pressed[sequence_pos] = hub.AnyButtonPressed();
    	    // use yields statements any time the hub is pausing or waiting
    	    yield(false);
    	}
    	while (!(pressed[sequence_pos] != 0) //0 if any touchpad is touched
    	        //0 if timed out
    			// no timeouts for now
    	        /*&& millis()  < timestampTouchpad + TIMEOUT_INTERACTIONS_MS*/);
  		// disable button sound
  		hub.SetButtonAudioEnabled(0);
    	hub.SetLights(hub.LIGHT_BTNS, 0, 0, 0); // turn off all touchpad lights
      // give the Hub a moment to finish playing the touchpad sound
      yield_sleep_ms(SOUND_TOUCHPAD_DELAY, false);

	    if (pressed[sequence_pos] != 0)
	    {
        Log.info("Touchpad touched");

        // Do lots of logging
        touchLog[touchLogIndex] = pressed[sequence_pos];
        touchLogTimes[touchLogIndex] = millis() - timestampBefore;
        touchLogIndex++;

        timeout = false;
        if (pressed[sequence_pos] == touchpad_sequence[sequence_pos]){
        	// correct touchpad touched in sequence
        	accurate = true;
        	Log.info("Correct");
        } else {
          Log.info("Miss");
          accurate = false;
          responseMisses++;

        	// Log.info("currentLevel: %u", currentLevel);
        	// Log.info("sequence_pos: %u", sequence_pos);
        	// Log.info("sequenceLength: %u", sequenceLength);

          // calculate End On Miss chance [0-100], see game-logic table for formulas
        	int eom = 0;
        	if (sequenceLength == 1) {
            eom = END_ON_MISS_CHANCE_1[currentLevel-1];
        	} else if ((sequenceLength - 1) == sequence_pos) {
        		eom = END_ON_MISS_CHANCE_2[((currentLevel-5)%16)];
        	} else if ((sequenceLength - 1) == (sequence_pos + 1)){
        		eom = END_ON_MISS_CHANCE_3[((currentLevel-5)%16)];
        	} else if ((sequenceLength - 1) > (sequence_pos + 1)) {
        		eom = 100;
        	}

        	Log.info("eom value: %u", eom);

        	// we have a miss, spin the wheel, rien ne vas plus
        	int rando = ((int)(rand() % 100));
        	Log.info("random number: %u",rando);
        	bool lucky = rando >= eom;
        	Log.info("lucky?: %u",lucky);

        	if (lucky){
        		Log.info("Touch tollerated, ignoring touch");

            // we got lucky, lets redo this one by decreasing sequence_pos
            sequence_pos--;

            // on level 2 and 3 we flash the correct pad on a miss
            if(currentLevel == 2 || currentLevel == 3){
              Log.info("Giving post-cue");
              hub.SetLightsRGB(
                touchpad_sequence[sequence_pos],
                TARGET_RESPONSE_INTENSITY_RED,
                TARGET_RESPONSE_INTENSITY_GREEN,
                TARGET_RESPONSE_INTENSITY_BLUE,
                10,5);
              yield_sleep_ms(200, false);
              hub.SetLights(hub.LIGHT_BTNS, 0, 0, 0); // turn off all touchpad lights
            }

            // we got lucky, lets redo this one by decreasing sequence_pos
            sequence_pos--;

        	} else {
        		// we lost, it's a miss
        		Log.info("It's a real miss");
        		break;
        	}
        }
	    } else {
	    	// we have a timeout
        Log.info("No touchpad pressed, timeout");
        accurate = false;
        timeout = true;
	    }
  	}
	} else {
		Log.info("We had a touch during presentation phase, automatic miss.");
		accurate = false;
		timeout = false;
	}

  if (accurate) {
    Log.info("Sequence correct");
    hub.PlayAudio(hub.AUDIO_POSITIVE, 60);
    // give the Hub a moment to finish playing the reward sound
    yield_sleep_ms(SOUND_AUDIO_POSITIVE_DELAY, false);

    foodtreatPresented = (((int)(rand() % 100)) <= REINFORCE_RATIO);
    if(foodtreatPresented){
      Log.info("Dispensing foodtreat");
      
      hub.PlayAudio(hub.AUDIO_POSITIVE, 60);
      // give the Hub a moment to finish playing the reward sound
      yield_sleep_ms(SOUND_AUDIO_POSITIVE_DELAY, false);

      do {
        foodtreatState=hub.PresentAndCheckFoodtreat(FOODTREAT_DURATION);
        yield(false);
      } while (foodtreatState!=hub.PACT_RESPONSE_FOODTREAT_NOT_TAKEN &&
        foodtreatState!=hub.PACT_RESPONSE_FOODTREAT_TAKEN);

      // Check if foodtreat was eaten
      if (foodtreatState == hub.PACT_RESPONSE_FOODTREAT_TAKEN){
        Log.info("Treat was eaten");
        foodtreatWasEaten = true;
      } else {
        Log.info("Treat was not eaten");
        foodtreatWasEaten = false;
      }
    } else {
      Log.info("No foodtreat this time (REINFORCE_RATIO)");
    }
  } else {
    if (!timeout) {
      hub.PlayAudio(hub.AUDIO_NEGATIVE, 60);
      // give the Hub a moment to finish playing the sound
      yield_sleep_ms(SOUND_AUDIO_NEGATIVE_DELAY, false);
      foodtreatWasEaten = false;
    }
  }

  // record time period for performance logging
  activityDuration = millis() - timestampBefore;

  if(!accurate && !timeout){
    // always do a retry when we the player got it wrong
    retrySequence = true;
  }

  // Send report
  // TODO this report might get too long for particle publish size limits
  if(!timeout){
    Log.info("Sending report");

    String extra ="{";
    extra += "\"targetSeq\":\"";
    for (int i = 0; i < sequenceLength; ++i){
        extra += convertBitfieldToLetter(touchpad_sequence[i]);
    }
    extra += "\",\"touchSeq\":\"";
    for (int i = 0; i < touchLogIndex; ++i){
        extra += convertBitfieldToLetter(touchLog[i]);
        if (i < (touchLogIndex -1)){extra += ",";}
    }
    extra += "\",\"touchTimes\":\"";
    for (int i = 0; i < touchLogIndex; ++i){
      extra += String(touchLogTimes[i]);
      if (i < (touchLogIndex -1)){extra += ",";}
    }
    extra += "\",\"presentMisses\":\"";
    extra += String(presentMisses);
    extra += "\",\"responseMisses\":\"";
    extra += String(responseMisses);
    if(presentMisses == 0){
      extra += "\",\"hintIntensity\":\"";
      extra += String((int)(hintIntensityMultipl*100)); // is the same for whole seq in one level
    }
    extra += "\",\"reinforceRatio\":\"";
    extra += String(REINFORCE_RATIO);
    extra += String::format("\",\"retryGame\":%c",retrySequence ? '1' : '0');

    extra += "}";

    // Log.info(extra);

    hub.Report(Time.format(gameStartTime,
                           TIME_FORMAT_ISO8601_FULL),  // play_start_time
               PlayerName,                             // player
               currentLevel,                           // level
               String(accurate),                       // result
               activityDuration,                       // duration
               foodtreatPresented,           // foodtreat_presented
               foodtreatWasEaten,  // foodtreatWasEaten
               extra               // extra field
    );
  }

  // keep track of performance
  if (!timeout){
    addResultToPerformanceHistory(accurate);
  }

  // adjust level according to performance
  if (countSuccesses() >= ENOUGH_SUCCESSES) {
    Log.info("Increasing level! %u", currentLevel);
    currentLevel++;
    resetPerformanceHistory();
  } else if (countMisses() >= TOO_MANY_MISSES) {
    if (currentLevel > 1)
    {
      Log.info("Decreasing level! %u", currentLevel);
      currentLevel--;
    }
    resetPerformanceHistory();
  }

  printPerformanceArray();

  if(!accurate && !timeout){
    // between interaction timeout if wrong
    yield_sleep_ms(INTER_GAME_DELAY, false);
  }

  hub.SetDIResetLock(false);  // allow DI board to reset if needed between interactions
  yield_finish();
  return true;
}


/**
 * Setup function
 * --------------
 */
void setup() {
  // Initializes the hub and passes the current filename as ID for reporting
  hub.Initialize(__FILE__);
}

/**
 * Main loop function
 * ------------------
 */
void loop()
{
  bool gameIsComplete = false;

  // Advance the device layer state machine, but with 20 ms max time
  // spent per loop cycle.
  hub.Run(20);

  // Play 1 interaction of the Learning Longer Sequences challenge
  // Will return true if level is done
  gameIsComplete = playSimon();

  if(gameIsComplete){
    // Interaction end
    return;
  }
}
