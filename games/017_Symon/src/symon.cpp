/**
  Symon 
  =====
  
  Symon is a version of the Simon memory game by Milton-Bradley, but for dogs,
  cats, pigs, chimps, and any other animal that can be taught to play it.
  
  ## Game basics
  
  1. To begin with, it starts with all three touchpads illuminated. The player
     can touch any of them to start the game.  
  2. **See**: A round begins with a single "knock" sound, followed by one or 
     more touchpads lighting up in a sequence, along with the touchpad's 
     associated sound.  
  3. **Do**: After the sequence has completed, a "double knock" sound will 
     indicate that it's the player's turn to repeat it.
    
  After the game has started, the three illuminated pads of Step 1 will only
  appear if the player doesn't respond. Otherwise, the game will continue
  immediately after the player's response, whether incorrect or correct. 
    
  ## Levels
    
  In Symon, the game starts at Level 10 -- the game is designed to follow
  Challenge 9: "Learning Longer Sequences" from the standard CleverPet Curriculum
    
  Levels 10-19 involve having your player remember sequences that involve one
  light. 
    
  Levels 20-29 involve your player remembering sequences that involve two
  lights. 
    
  Levels 30-39 require remembering a sequence of three lights, and so. In
  general, it's possible to tell how long a sequence is that a player is 
  working on by looking at the digit in the 10s place in the level number. 
  Thus, as the levels go up, the game becomes more challenging. 
    
  While working on a given sequence length, x0 through x9 change the difficulty
  of the game to help train your player. Two aspects of the game get modified to
  improve the player's training: 

  1. "Exit on Miss probability" -- as the game shifts from x0 to x9, the game 
     becomes more "strict", tolerating fewer incorrect touches as the player 
     guesses which touchpad is the right one. We describe this as an increased 
     probability of exiting the round when a player gets a touch wrong.  
  2. "Delay to See phase" -- as the game shifts from x0 to x9, the maximum 
     possible delay before the "see" phase begins increases, requiring the 
     player to wait longer. The shortest possible delay is 50 ms, with the 
     longest possible one being 2300 ms.
  
    Game levels in detail: http://clvr.pt/symon-game-specification
  
    Authors: CleverPet, Inc.  
             Jelmer Tiete <jelmer@tiete.be>
  
    Copyright 2019 
    Licensed under the AGPL 3.0
*/

#include <hackerpet.h>
#include <algorithm>

// Set this to the name of your player (dog, cat, etc.)
const char PlayerName[] = "Salk";

/**
 * Challenge settings
 * -------------
 *
 * These constants (capitalized) and variables (camelCase) define the
 * gameplay
 */
int currentLevel = 20; // LEVELS START AT 10
const int MIN_LEVEL =      10;   // Number of past interactions to look at for performance
const int HISTORY_LENGTH=      7;   // Number of past interactions to look at for performance
const int ENOUGH_SUCCESSES=    6;   // if successes >= ENOUGH_SUCCESSES level-up
const int TOO_MANY_MISSES=     4;   // if num misses >= TOO_MANY_MISSES level-down
const int REINFORCE_RATIO =      100; // the foodtreat reinforcement ratio [0-100] 100:always foodtreat
// LED colors and intensities
const int CUE_LIGHT_PRESENT_INTENSITY_RED = 99; // [0-99] // cue / status light is yellow in present phase
const int CUE_LIGHT_PRESENT_INTENSITY_GREEN = 99; // [0-99]
const int CUE_LIGHT_PRESENT_INTENSITY_BLUE = 0; // [0-99]
const int CUE_LIGHT_RESPONSE_INTENSITY_RED = 99; // [0-99] // cue / status light is white in response phase
const int CUE_LIGHT_RESPONSE_INTENSITY_GREEN = 99; // [0-99]
const int CUE_LIGHT_RESPONSE_INTENSITY_BLUE = 99; // [0-99]
const int SLEW = 0; // slew for all lights [0-99]
const int START_INTENSITY_RED = 40; // [0-99]
const int START_INTENSITY_GREEN = 40; // [0-99]
const int START_INTENSITY_BLUE = 40; // [0-99]
const int TARGET_PRESENT_INTENSITY_RED = 80; // [0-99]
const int TARGET_PRESENT_INTENSITY_GREEN = 80; // [0-99]
const int TARGET_PRESENT_INTENSITY_BLUE = 80; // [0-99]
const int TARGET_RESPONSE_INTENSITY_RED = 80; // [0-99]
const int TARGET_RESPONSE_INTENSITY_GREEN = 80; // [0-99]
const int TARGET_RESPONSE_INTENSITY_BLUE = 80; // [0-99]
const int TARGET_RESPONSE_MISS_INTENSITY_RED = 99; // [0-99]
const int TARGET_RESPONSE_MISS_INTENSITY_GREEN = 99; // [0-99]
const int TARGET_RESPONSE_MISS_INTENSITY_BLUE = 0; // [0-99]
const int HINT_INTENSITY_MULTIPL_1[] = {100,30,5,0,0,0,0,0,0,0}; // for level 10-19
const int HINT_INTENSITY_MULTIPL_2[] = {0,0,0,0,0,0,0,0,0,0}; // for level 20 on
// Volume
const int AUDIO_VOLUME = 60; //[0-99]
// Delays and wait times
const unsigned long FOODTREAT_DURATION = 4000; // (ms) how long to present foodtreat
const unsigned long TIMEOUT_INTERACTIONS_MS = 10000; // (ms) how long to wait until restarting the
                                                    // interaction
const unsigned long INTER_GAME_DELAY = 5000; // timeout inbetween games on miss
const int RESPONSE_PHASE_WAIT_TIME[] = {250,250,250,250,500,750,1000,1500,1750,2250}; // for all levels
// Chance calculations
const int END_ON_MISS_CHANCE_1[] = {100,0,20,30,30,40,60,80,100,100}; // for level 10 - 19
const int END_ON_MISS_CHANCE_2[] = {0,20,30,40,60,60,60,70,90,100}; // for level 20 on and seq_pos == seq_lenght-2
const int END_ON_MISS_CHANCE_3[] = {0,10,20,30,40,50,60,70,90,100}; // for level 20 on and  seq_pos == seq_length-1
/**
 * Global variables and constants
 * ------------------------------
 */
const unsigned long SOUND_AUDIO_POSITIVE_DELAY = 350; // (ms) delay for reward sound
const unsigned long SOUND_AUDIO_NEGATIVE_DELAY = 350;
const unsigned long SOUND_TOUCHPAD_DELAY = 300; // (ms) delay for touchpad sound
const unsigned long SOUND_DO_DELAY = 150; // (ms) delay for reward sound

const unsigned long HINT_WAIT = 9000;

const unsigned int DEFAULT_CORRECTION_EXIT_PERCENT = 20;
const unsigned int FOCUS_CORRECTION_EXIT_PERCENT = 10;
const unsigned int FOCUS_SUCCESS_EXIT_PERCENT = 15;
const unsigned int STREAK_FOOD_MAX = 3;

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

int giveFoodtreat(String command)
{
    Log.info("Received giveFoodtreat.");
    int foodtreat_duration = 5000;
    if (command.length() > 0)
    {
        foodtreat_duration = command.toInt();
    }

    Log.info("Foodtreat present duration is %d", foodtreat_duration);

    hub.PresentAndCheckFoodtreat(foodtreat_duration);
    return 1;
}

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
bool playSymon(){
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
  static int hintIntensityMultipl = 0;
  static unsigned char touchpad_sequence[SEQUENCE_LENGTHMAX]={};
  static unsigned char pressed[SEQUENCE_LENGTHMAX] = {};
  static unsigned char touchLog[LOG_LENGTH_MAX] = {};  // could end up longer than sequence length, should use vector
  static unsigned char tmpPressed = 0;
  static int sequenceLength = 0; // sequence length (gets calculated)
  static int touchLogIndex = 0;
  static int presentMisses = 0; // logging error touches during present phase
  static int responseMisses = 0; // logging error touches during response phase
  static bool accurate = false;
  static bool timeout = false;
  static bool foodtreatPresented = false; // store if foodtreat was presented
  static bool foodtreatWasEaten = false; // store if foodtreat was eaten in last interaction
  static int retryCounter = 0; // do not re-initialize
  static int prevRetryCounter = 0; // do not re-initialize
  static int streakCounter = 0; // do not re-initialize
  static bool focusPuzzle = false; // do not re-initialize
  static bool dodoSoundPlayed = false;
  static int timedHintCount = 0; // to keep the size of the number of perf numbers to consider
  static int i_i = 0; // for iterating over yields
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
  timedHintCount = 0;
  tmpPressed = 0;

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
  Log.info("Starting new \"Symon\" challenge");

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
  sequenceLength = (currentLevel/10); // see game-logic chart

  if(!focusPuzzle) // new game
  {

    /* 
    If player gets it wrong first time around, retry with 20% chance of exit 

    If player gets it right first time around, show the player the same one again (focus mode)

      If they then get it wrong on a subsequent try, retry with 10% chance of exit

      If they then get it right on a subsequent try, 
        if they got the previous x right, give them x % 3 extra food, retry with 15% chance of exit. 
    */

    /* // old code
    // only reset the game if they got it right immediately. 
    if ( !(retryCounter == 0 && prevRetryCounter > 3) 
            && (random(0, 100) < 40))
    {
    */
    // fill touchpad_sequence
    for (int i = 0; i < sequenceLength; ++i)
    {
        /*if (random(0,100) < 50) 
        { // bias correct
          touchpad_sequence[0] = hub.BUTTON_LEFT;//touchpads[0];
        }
        else 
        {*/
        random_shuffle(&touchpads[0], &touchpads[3]);
        touchpad_sequence[i] = touchpads[0];
        //}
    }
    //}
    retryCounter = 0; // seems redundant
  } else { // same game
    Log.info("Doing a retry game");
    if (retryCounter == 4)
    {
        hub.ResetFoodMachine();
        yield_sleep_ms(400, false);
        yield_wait_for((hub.IsReady()
                && hub.FoodmachineState() == hub.FOODMACHINE_IDLE), false);
    }
  }

  Log.info("Current level: %u, sequence length: %u, successes: %u, misses: %u",
  currentLevel, sequenceLength, countSuccesses(), countMisses());

  // Log sequence
  // String seqString = "Sequence: ";
  // for (int i = 0; i < sequenceLength; ++i){
  //     seqString += convertBitfieldToLetter(touchpad_sequence[i]);
  //     if (i < (sequenceLength -1)){seqString += ",";}
  // }
  // Log.info(seqString);

  // turn off the button sounds, this isn't part of the game yet
  hub.SetButtonAudioEnabled(0);

  // turn off the cue light
  hub.SetLights(hub.LIGHT_CUE,0,0,0);

  // turn on the touchpad lights at start intensity
  hub.SetLightsRGB(
    hub.LIGHT_BTNS,
    START_INTENSITY_RED,
    START_INTENSITY_GREEN,
    START_INTENSITY_BLUE,
    SLEW);

  // wait until: a button is currently pressed
  yield_wait_for(hub.AnyButtonPressed(), false);
  if(hub.AnyButtonPressed()){
    touchLog[touchLogIndex] = hub.AnyButtonPressed();
    touchLogTimes[touchLogIndex] = 0;
    touchLogIndex++;
    // Record start timestamp for performance logging
    timestampBefore = millis();
  }

  // turn off all touchpad lights
  hub.SetLights(hub.LIGHT_BTNS, 0, 0, 0);

  // slight delay to prevent double presses to show up as a miss
  yield_sleep_ms(100, false);

  // wait until: no button is currently pressed
  yield_wait_for((!hub.AnyButtonPressed()), false);


//------------------------------------------------------------------------------
    // SEE PHASE

  // turn off the button sounds - we're doing these manually
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
      hub.PlayAudio(buttonToAudio(touchpad_sequence[sequence_pos]), AUDIO_VOLUME);
      // give the Hub a moment to finish playing the sound and detect touches
      yield_wait_for_with_timeout(hub.AnyButtonPressed(), SOUND_TOUCHPAD_DELAY+200,false);
      if(hub.AnyButtonPressed()){break;}

      // turn off touchpad light
      hub.SetLights(touchpad_sequence[sequence_pos],0,0,SLEW);
    }
  }

  // wait random time before response phase (min:50 max:RESPONSE_PHASE_WAIT_TIME) and detect touches
  yield_wait_for_with_timeout(hub.AnyButtonPressed(),
    (random(0,RESPONSE_PHASE_WAIT_TIME[currentLevel % 10]))+50,
    false);

  // yield_sleep_ms(1300, false);
  // wait time before response phase and detect touches
  yield_wait_for_with_timeout(hub.AnyButtonPressed(), random(50,2800),false);
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
    // DO PHASE

  // wait until: no button is currently pressed
  yield_wait_for((!hub.AnyButtonPressed()), false);


  if(!presentMisses){
    // delay after turning cue light to white
      yield_sleep_ms(400, false);

    // start response detection
    for (sequence_pos = 0; sequence_pos < sequenceLength; ++sequence_pos)
    {
      // wait until: no button is currently pressed
      yield_wait_for((!hub.AnyButtonPressed()), false);

      // turn on response hint, see game logic table for intensity calculation
      if (currentLevel < 20){
       hintIntensityMultipl = HINT_INTENSITY_MULTIPL_1[currentLevel % 10];
      } else {
       hintIntensityMultipl = HINT_INTENSITY_MULTIPL_2[currentLevel % 10];
      }

      // Log.info("currentLevel: %u", currentLevel);
      // Log.info("sequence_pos: %u", sequence_pos);
      // Log.info("hintIntensityMultipl: %d", hintIntensityMultipl);

      // randomize hint intensities higher than 5
      if (hintIntensityMultipl > 5)
      {
        int newHintIntensityMultipl = random(5,(hintIntensityMultipl*2));
        // Log.info("newHintIntensityMultipl: %d", newHintIntensityMultipl);
        if (newHintIntensityMultipl < hintIntensityMultipl)
          hintIntensityMultipl = newHintIntensityMultipl;
      }

      // Log.info("randomized hintIntensityMultipl: %d", hintIntensityMultipl);

      hub.SetLightsRGB(
        touchpad_sequence[sequence_pos],
        ((TARGET_RESPONSE_INTENSITY_RED * hintIntensityMultipl) / 100),
        ((TARGET_RESPONSE_INTENSITY_GREEN * hintIntensityMultipl) / 100),
        ((TARGET_RESPONSE_INTENSITY_BLUE * hintIntensityMultipl) / 100),
        SLEW);

      // on first light in sequence, play DODO sound
      if (sequence_pos == 0 && !dodoSoundPlayed){
        // play DODO sound
        hub.PlayAudio(hub.AUDIO_DO, 90);
        yield_sleep_ms(SOUND_DO_DELAY, false);
        hub.PlayAudio(hub.AUDIO_DO, 90);
        hub.SetLightsRGB(
          hub.LIGHT_CUE,
          CUE_LIGHT_RESPONSE_INTENSITY_RED,
          CUE_LIGHT_RESPONSE_INTENSITY_GREEN,
          CUE_LIGHT_RESPONSE_INTENSITY_BLUE,
          SLEW);
        yield_sleep_ms(SOUND_DO_DELAY, false);
        dodoSoundPlayed = true;
        // illuminate cue light White
      }

      timestampTouchpad = millis();
      do
      {
          // detect any buttons currently pressed
          pressed[sequence_pos] = hub.AnyButtonPressed();
          /*
          // use yields statements any time the hub is pausing or waiting
          if ( hintIntensityMultipl == 0 && ((millis() - timestampBefore) > HINT_WAIT * (timedHintCount + 1) ))
          {
            timedHintCount++;
            hub.SetLightsRGB(
              touchpad_sequence[sequence_pos],
              TARGET_RESPONSE_INTENSITY_RED/4,
              TARGET_RESPONSE_INTENSITY_GREEN/4,
              TARGET_RESPONSE_INTENSITY_BLUE/4,
              SLEW);
            yield_sleep_ms(60, false);
            hub.SetLights(hub.LIGHT_BTNS, 0, 0, 0); // turn off all touchpad lights
          }*/

          yield(false);
      }
      while (!(pressed[sequence_pos] != 0) //0 if any touchpad is touched
              //0 if timed out
          // no timeouts for now
              && millis()  < timestampTouchpad + TIMEOUT_INTERACTIONS_MS);

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
          // blink correct touchpad
          hub.SetLightsRGB(
            pressed[sequence_pos],
            TARGET_RESPONSE_INTENSITY_RED,
            TARGET_RESPONSE_INTENSITY_GREEN,
            TARGET_RESPONSE_INTENSITY_BLUE,
            SLEW);
          yield_sleep_ms(20, false);
          hub.SetLights(hub.LIGHT_BTNS, 0, 0, 0); // turn off all touchpad lights
          // play touchpad sound
          hub.PlayAudio(buttonToAudio(pressed[sequence_pos]), AUDIO_VOLUME);
          yield_sleep_ms(SOUND_TOUCHPAD_DELAY-50, false);
          accurate = true;
          Log.info("Correct");
        } else {
          // wrong touchpad touched
          // blink wrong touchpad
          hub.SetLightsRGB(
            pressed[sequence_pos],
            TARGET_RESPONSE_MISS_INTENSITY_RED,
            TARGET_RESPONSE_MISS_INTENSITY_GREEN,
            TARGET_RESPONSE_MISS_INTENSITY_BLUE,
            SLEW);
          yield_sleep_ms(20, false);
          hub.SetLights(hub.LIGHT_BTNS, 0, 0, 0); // turn off all touchpad lights          
          Log.info("Miss");
          accurate = false;
          responseMisses++;

          // calculate End On Miss chance [0-100], see game-logic table for formulas
          int eom = 0;
          if (sequenceLength == 1) {
            eom = END_ON_MISS_CHANCE_1[currentLevel % 10];
          } else if (sequence_pos == (sequenceLength - 2)) {
            eom = END_ON_MISS_CHANCE_2[currentLevel % 10];
          } else if (sequence_pos == (sequenceLength - 1)){
            eom = END_ON_MISS_CHANCE_3[currentLevel % 10];
          } else if (sequence_pos < (sequenceLength - 2)) {
            eom = 100;
          }

          // Log.info("currentLevel: %u", currentLevel);
          // Log.info("sequence_pos: %u", sequence_pos);
          // Log.info("sequenceLength: %u", sequenceLength);
          // Log.info("eom value: %u", eom);

          // we have a miss, spin the wheel, rien ne vas plus
          int rando = ((int)(rand() % 100));
          Log.info("random number: %u",rando);
          bool lucky = rando >= eom;
          Log.info("lucky?: %u",lucky);

          if (lucky){
            Log.info("Touch tollerated, ignoring touch");

            // on level 11 and 12 we flash the correct pad on a miss
            if(currentLevel == 11 || currentLevel == 12){
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
        break;
      }
    }
  } else {
    Log.info("We had a touch during presentation phase, automatic miss.");
    accurate = false;
    timeout = false;
  }

  // game end, turn off cue light
  hub.SetLights(hub.LIGHT_CUE,0,0,0);

  if (accurate) {
    Log.info("Sequence correct");
    hub.PlayAudio(hub.AUDIO_POSITIVE, AUDIO_VOLUME);
    // give the Hub a moment to finish playing the reward sound
    yield_sleep_ms(SOUND_AUDIO_POSITIVE_DELAY, false);

    foodtreatPresented = (((int)(rand() % 100)) <= REINFORCE_RATIO);

    if(foodtreatPresented)
    {
      /*
      for (i_i = 0; i_i < (streakCounter % (STREAK_FOOD_MAX + 1)); i_i++)
      {
        Log.info("Dispensing extra food to dish");
        hub.PlayAudio(hub.AUDIO_POSITIVE, AUDIO_VOLUME);
        // give the Hub a moment to finish playing the reward sound
        yield_sleep_ms(SOUND_AUDIO_POSITIVE_DELAY, false);

        hub.ResetFoodMachine();
        yield_sleep_ms(400, false);
        yield_wait_for((hub.IsReady()
                && hub.FoodmachineState() == hub.FOODMACHINE_IDLE), false);
      }
      */
      Log.info("Dispensing foodtreat");

      hub.PlayAudio(hub.AUDIO_POSITIVE, AUDIO_VOLUME);
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
        hub.ResetFoodMachine();
        yield_sleep_ms(300, false);
        yield_wait_for((hub.IsReady()
          && hub.FoodmachineState() == hub.FOODMACHINE_IDLE), false);
      }
    } else {
      Log.info("No foodtreat this time (REINFORCE_RATIO)");
    }
  } else {
    if (!timeout) {
      hub.PlayAudio(hub.AUDIO_NEGATIVE, AUDIO_VOLUME);
      // give the Hub a moment to finish playing the sound
      yield_sleep_ms(SOUND_AUDIO_NEGATIVE_DELAY, false);
      foodtreatWasEaten = false;
    }
  }

  // record time period for performance logging
  activityDuration = millis() - timestampBefore;

  // Send report
  // TODO this report might get too long for particle publish size limits
  if(!timeout){
    Log.info("Sending report");

    String extra ="{";
    extra += "\"targetSeq\":\"";
    for (int i = 0; i < sequenceLength; ++i){
        extra += convertBitfieldToLetter(touchpad_sequence[i]);
        if (i < (sequenceLength -1)){extra += ",";}
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
      extra += String(hintIntensityMultipl); // is the same for whole seq in one level
    }
    extra += "\",\"reinforceRatio\":\"";
    extra += String(REINFORCE_RATIO);
    extra += String::format("\",\"retryGame\":%d", retryCounter);

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

  // keep track of performance and retry games
  if(!timeout){

    addResultToPerformanceHistory(accurate);

    // always do a retry when we the player got it wrong else reset it
    prevRetryCounter = retryCounter;
    if(!accurate)
    {
      if ((!focusPuzzle && random(0, 100) > DEFAULT_CORRECTION_EXIT_PERCENT) 
          || (focusPuzzle && random(0, 100) > FOCUS_CORRECTION_EXIT_PERCENT))
      {
        retryCounter++;
        focusPuzzle = true;
      }
      else
      {
        retryCounter = 0;
        focusPuzzle = false;
      }
      
      streakCounter = 0;
    }
    else // successful interaction!
    {
      if (streakCounter > 0 && random(0, 100) < FOCUS_SUCCESS_EXIT_PERCENT)  // subsequent success -- stop repeat this % of the time
      {
        focusPuzzle = false;
      } 
      else
      {
        focusPuzzle = true;
      }
      streakCounter++;
      retryCounter = 0;
    }
  }

  // adjust level according to performance
  if (countSuccesses() >= ENOUGH_SUCCESSES) {
    Log.info("Increasing level! %u", currentLevel);
    currentLevel++;
    resetPerformanceHistory();
  } else if (countMisses() >= TOO_MANY_MISSES) {
    if (currentLevel > MIN_LEVEL)
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
  Particle.function("giveFoodtreat", giveFoodtreat);
  // Initializes the hub and passes the current filename as ID for reporting
  hub.Initialize(__FILE__);
  hub.ResetFoodMachine();
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
  gameIsComplete = playSymon();

  if(gameIsComplete){
    // Interaction end
    return;
  }
}
