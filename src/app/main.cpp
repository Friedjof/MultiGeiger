#include <Arduino.h>

#include "app/controller.hpp"

MultiGeigerController controller;  // Global instance (used by config callbacks)

void setup() {
  controller.begin();
}

void loop() {
  controller.loopOnce();
}
