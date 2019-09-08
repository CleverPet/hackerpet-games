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
  bool wait_after_game;
  unsigned int timeout_ms;
  unsigned int loss_timer_ms;
  unsigned char colors[8][2];
  unsigned int rounds_per_game;
  std::function<void(unsigned char*)> on_round_start;
  std::function<void(unsigned char*)> periodic_update;
  std::function<void(unsigned char*, int)> on_touch;
  std::function<bool(int)> did_pet_win;
  std::function<void()> on_pet_win;
  std::function<bool(int)> did_pet_lose;
  std::function<void()> on_pet_lose;
};

// enables simultaneous execution of application and system thread
SYSTEM_THREAD(ENABLED);

void play(Game g) {
  yield_begin();
//   yield_wait_for((hub.IsReady() && hub.FoodmachineState() == hub.FOODMACHINE_IDLE && not hub.AnyButtonPressed()),);
  yield_wait_for((not hub.AnyButtonPressed()),);

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
        if (rounds == g.rounds_per_game) {
          // shut off lights and emit sound to signal game end
          hub.SetLights(hub.LIGHT_BTNS, 0, 0, 0);
          hub.PlayAudio(hub.AUDIO_POSITIVE, 80);
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
        // shut off lights and emit sound to signal game end
        hub.SetLights(hub.LIGHT_BTNS, 0, 0, 0);
        hub.PlayAudio(hub.AUDIO_NEGATIVE, 80);
        yield_sleep_ms(g.loss_timer_ms,);
        g.on_pet_lose();
        if (g.wait_after_game) {
          break;
        }
        rounds = 0;
        restart = true;
      }
      else {
        // update lights and emit sound
        for (int i = 0; i < 3; i++) {
          if (temp_colors[i] != colors[i]) {
            hub.SetLights(LIGHTS[i], g.colors[colors[i]][0], g.colors[colors[i]][1], 0);
          }
        }
        hub.PlayAudio(hub.AUDIO_DO, 60);
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
