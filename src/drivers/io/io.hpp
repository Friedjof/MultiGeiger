// Hardware IO helpers: hardware detection, switches, speaker (incl. LED tick), timers.

#pragma once

#include <Arduino.h>
#include <driver/mcpwm.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "core/core.hpp"
#include "config/config.hpp"

// DIP switch related code
typedef struct switches {
  unsigned int speaker_on: 1;  // SW0
  unsigned int display_on: 1;  // SW1
  unsigned int led_on: 1;      // SW2
  unsigned int ble_on: 1;      // SW3
} Switches;

bool init_hwtest(void);

void setup_switches(bool isLoraBoard);
Switches read_switches(void);

void setup_speaker(bool playSound, bool led_tick, bool speaker_tick);
void tick_enable(bool enable);
void tick(bool high);
void alarm();

void setup_recharge_timer(void (*isr_recharge)(), int period_us);
void setup_audio_timer(void (*isr_audio)(), int period_us);

// Thin OO wrapper for IO-related helpers.
class IoModule {
public:
  bool detectLoRa() { return init_hwtest(); }
  void setupSwitches(bool isLoraBoard) { setup_switches(isLoraBoard); }
  Switches readSwitches() { return read_switches(); }

  void setupSpeaker(bool playSound, bool ledTick, bool speakerTick) { setup_speaker(playSound, ledTick, speakerTick); }
  void enableTick(bool enable) { tick_enable(enable); }
  void doTick(bool high) { tick(high); }
  void triggerAlarm() { alarm(); }

  void setupRechargeTimer(void (*isr)(), int period_us) { setup_recharge_timer(isr, period_us); }
  void setupAudioTimer(void (*isr)(), int period_us) { /* no-op: audio handled in task */ }
};
