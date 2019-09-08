#include "Functional.h"

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
    .loss_timer_ms = 6000,
    .colors = {
      {90, 00},
      {00, 90},
      {70, 70},
    },
    .rounds_per_game = 2,
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
      return tries >= 2;
    },
    .on_pet_lose = []() {},
  });
}