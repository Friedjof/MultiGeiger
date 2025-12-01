#include <Arduino.h>

#include "app/controller.hpp"

static MultiGeigerController controller;

void setup() {
  controller.begin();
}

void loop() {
  controller.loopOnce();
}
