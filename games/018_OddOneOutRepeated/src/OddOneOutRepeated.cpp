// This #include statement was automatically added by the Particle IDE.
#include <hackerpet.h>
#include <functional>

HubInterface hub;

const unsigned long FOODTREAT_DELAY = 600; // (ms) delay for reward sound
const unsigned long TOUCHPAD_DELAY = 300; // (ms) delay for touchpad sound
const unsigned long FOODTREAT_DURATION = 6000; // (ms) how long to present foodtreat
const unsigned long SOUND_DELAY = 600;
const unsigned int DIM = 30;
const unsigned long LIGHT_REFRESH = 20000;

const unsigned int LIGHTS[3] = { hub.LIGHT_LEFT, hub.LIGHT_MIDDLE, hub.LIGHT_RIGHT };

struct Game {
  bool wait_after_game; // this is whether the hub should return to the waiting state between games
  unsigned int timeout_ms; // thie is how long without interaction the hub will wait before returning to the waiting state
  unsigned int loss_timer_ms; // this is how long after a pet's loss that the hub waits before starting a new game or returns to the waiting state
  unsigned char colors[8][2]; // this is an array of BY values for the colors that will be used for the game
  unsigned int rounds_per_game; // this is the number of times the pet needs to win before being presented a foodtreat
  unsigned int tries_per_game; // this is the number of times the pet is allowed to lose before restarting the game
  // the functions below are executed upon certain events being triggered
  // unsigned char* arguments are arrays to be modified in-place with the index of the color that the lights should be
  // for example: colors = [[0, 0], [255, 255]] and modifying this argument to be [0, 1, 0] will cause the lights to be off, white, and off, respectively
  // the int argument is the index of the button that was pressed by the pet
  std::function<void(unsigned char*)> on_round_start; // initialize the game
  std::function<void(unsigned char*)> periodic_update; // updates that happen each time this game loop runs
  std::function<void(unsigned char*, int)> on_touch; // updates that happen whenever a pet presses a button
  std::function<bool(int)> did_pet_win; // condition check for if the pet won the round
  std::function<void()> on_pet_win; // updates that happen whenever a pet wins a round
  std::function<bool(int)> did_pet_lose; // condition check for if the pet lost the round
  std::function<void()> on_pet_lose; // updates that happen whenever a pet loses a round
};

// enables simultaneous execution of application and system thread
SYSTEM_THREAD(ENABLED);

void play(Game g) {
  yield_begin();
  yield_wait_for((hub.IsReady() && hub.FoodmachineState() == hub.FOODMACHINE_IDLE && not hub.AnyButtonPressed()),);

  static bool waiting = true;
  static bool restart;
  static int rounds;
  static int press;
  static int foodtreat_state;
  static int turn_timestamp;
  static unsigned char colors[3] = {};
  static unsigned char temp_colors[3] = {};
  static unsigned long refresh = millis();

  // waiting
  hub.SetLights(hub.LIGHT_BTNS, DIM, DIM, 0);
  for (;;) {
    // periodically refresh lights
    if (millis() > refresh + LIGHT_REFRESH) {
      hub.SetLights(hub.LIGHT_BTNS, DIM, DIM, 0);
      refresh = millis();
    }

    // wait until any valid button press
    yield();
    press = hub.AnyButtonPressed();
    if (press == hub.BUTTON_LEFT || press == hub.BUTTON_MIDDLE || press == hub.BUTTON_RIGHT) {
      yield_sleep_ms(TOUCHPAD_DELAY,);
      yield_wait_for((!hub.AnyButtonPressed()),);
      break;
    }
  }

  // reinitializing
  restart = true;
  rounds = 0;
  turn_timestamp = millis();
  for (;;) {
    if (restart) {
      rounds++;

      // reinitialize game for each round
      press = 0;
      foodtreat_state = 0;
      colors[0] = 0;
      colors[1] = 0;
      colors[2] = 0;
      hub.SetButtonAudioEnabled(true);
      hub.SetDIResetLock(true);
      g.on_round_start(colors);

      hub.SetLights(hub.LIGHT_LEFT, g.colors[colors[0]][0], g.colors[colors[0]][1], 0);
      hub.SetLights(hub.LIGHT_MIDDLE, g.colors[colors[1]][0], g.colors[colors[1]][1], 0);
      hub.SetLights(hub.LIGHT_RIGHT, g.colors[colors[2]][0], g.colors[colors[2]][1], 0);

      restart = false;
    }

    // game logic updating and optimize light updates
    for (int i = 0; i < 3; i++) {
      temp_colors[i] = colors[i];
    }
    g.periodic_update(colors);
    for (int i = 0; i < 3; i++) {
      if (temp_colors[i] != colors[i]) {
        hub.SetLights(LIGHTS[i], g.colors[colors[i]][0], g.colors[colors[i]][1], 0);
      }
    }

    // periodically refresh lights
    if (millis() > refresh + LIGHT_REFRESH) {
      hub.SetLights(hub.LIGHT_LEFT, g.colors[colors[0]][0], g.colors[colors[0]][1], 0);
      hub.SetLights(hub.LIGHT_MIDDLE, g.colors[colors[1]][0], g.colors[colors[1]][1], 0);
      hub.SetLights(hub.LIGHT_RIGHT, g.colors[colors[2]][0], g.colors[colors[2]][1], 0);
      refresh = millis();
    }

    // handle only valid button presses
    yield();
    press = hub.AnyButtonPressed();
    if (press == hub.BUTTON_LEFT || press == hub.BUTTON_MIDDLE || press == hub.BUTTON_RIGHT) {
      // convert press from bit flag to 0-index
      if (press == hub.BUTTON_LEFT) {
        press = 0;
      }
      else if (press == hub.BUTTON_MIDDLE) {
        press = 1;
      }
      else if (press == hub.BUTTON_RIGHT) {
        press = 2;
      }
      yield_sleep_ms(TOUCHPAD_DELAY,);
      yield_wait_for((!hub.AnyButtonPressed()),);

      turn_timestamp = millis();
      // game logic updating and optimize light updates
      for (int i = 0; i < 3; i++) {
        temp_colors[i] = colors[i];
      }
      g.on_touch(colors, press);
      if (g.did_pet_win(press)) {
        g.on_pet_win();
        hub.PlayAudio(hub.AUDIO_POSITIVE, 80);
        if (rounds == g.rounds_per_game) {
          // shut off lights and emit sound to signal game end
          hub.SetLights(hub.LIGHT_BTNS, 0, 0, 0);
          yield_sleep_ms(FOODTREAT_DELAY,);
          // dispense treat
          do {
            foodtreat_state = hub.PresentAndCheckFoodtreat(FOODTREAT_DURATION);
            yield();
          }
          while (foodtreat_state != hub.PACT_RESPONSE_FOODTREAT_NOT_TAKEN && foodtreat_state != hub.PACT_RESPONSE_FOODTREAT_TAKEN);
          if (g.wait_after_game) {
            break;
          }
          rounds = 0;
        }
        restart = true;
      }
      else if (g.did_pet_lose(press)) {
        g.on_pet_lose();
        hub.PlayAudio(hub.AUDIO_NEGATIVE, 80);
        if (rounds == g.tries_per_game) {
          // shut off lights and emit sound to signal game end
          hub.SetLights(hub.LIGHT_BTNS, 0, 0, 0);
          yield_sleep_ms(g.loss_timer_ms,);
          if (g.wait_after_game) {
            break;
          }
          rounds = 0;
        }
        restart = true;
      }
      else {
        // update lights and emit sound
        hub.PlayAudio(hub.AUDIO_DO, 60);
        for (int i = 0; i < 3; i++) {
          if (temp_colors[i] != colors[i]) {
            hub.SetLights(LIGHTS[i], g.colors[colors[i]][0], g.colors[colors[i]][1], 0);
          }
        }
        yield_sleep_ms(TOUCHPAD_DELAY,);
      }
    }
    else if (millis() > turn_timestamp + g.timeout_ms) {
      waiting = true;
      break;
    }
  }

  hub.SetButtonAudioEnabled(false);
  hub.SetDIResetLock(false);

  yield_finish();
}

//
// Game configuration is below this
//

void setup() {
  hub.Initialize(__FILE__);
}

int color_a;
int color_b;
int answer;
int tries;

void loop() {
  hub.Run(20);

  play({
    .wait_after_game = true,
    .timeout_ms = 30000,
    .loss_timer_ms = 10000,
    .colors = {
      {90, 00},
      {00, 90},
      {70, 70},
    },
    .rounds_per_game = 3,
    .tries_per_game = 2,
    .on_round_start = [](unsigned char* colors) {
      do {
        color_a = random(0, 3);
        color_b = random(0, 3);
      }
      while (color_a == color_b);
      answer = random(0, 3);
      colors[0] = color_a;
      colors[1] = color_a;
      colors[2] = color_a;
      colors[answer] = color_b;
      tries = 0;
    },
    .periodic_update = [](unsigned char* colors) {},
    .on_touch = [](unsigned char* colors, int pad) {
      tries++;
    },
    .did_pet_win = [](int pad) {
      return pad == answer;
    },
    .on_pet_win = []() {},
    .did_pet_lose = [](int pad) {
      return tries >= 3;
    },
    .on_pet_lose = []() {},
  });
}
