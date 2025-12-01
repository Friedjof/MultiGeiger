#include "io.hpp"
#include "driver/gpio.h"

// Hardware detection pin comes from config.hpp

bool init_hwtest(void) {
  pinMode(HWTESTPIN, INPUT_PULLUP);
  delay(200);
  if (!digitalRead(HWTESTPIN)) {      // low => LoRa chip detected
    return true;
  }
  return false;
}

// Switch handling

// Inputs for the switches
static unsigned int PIN_SWI_0, PIN_SWI_1, PIN_SWI_2, PIN_SWI_3;

void setup_switches(bool isLoraBoard) {
  if (isLoraBoard) {
    PIN_SWI_0 = 36;
    PIN_SWI_1 = 37;
    PIN_SWI_2 = 38;
    PIN_SWI_3 = 39;
  } else {
    PIN_SWI_0 = 39;
    PIN_SWI_1 = 38;
    PIN_SWI_2 = 37;
    PIN_SWI_3 = 36;
  };
  pinMode(PIN_SWI_0, INPUT);  // These pins DON'T HAVE PULLUPS!
  pinMode(PIN_SWI_1, INPUT);
  pinMode(PIN_SWI_2, INPUT);
  pinMode(PIN_SWI_3, INPUT);

  Switches s = read_switches();
  log(DEBUG, "Switches: SW0 speaker_on %d,  SW1 display_on %d,  SW2 led_on: %d,  SW3 ble_on: %d",
      s.speaker_on, s.display_on, s.led_on, s.ble_on);
}

Switches read_switches() {
  Switches s;

  // Read Switches (active LOW!)
  s.speaker_on = !digitalRead(PIN_SWI_0);
  s.display_on = !digitalRead(PIN_SWI_1);
  s.led_on = !digitalRead(PIN_SWI_2);
  s.ble_on = !digitalRead(PIN_SWI_3);

  return s;
}

// Speaker / LED tick handling

// Speaker pins come from config.hpp

// shall the speaker / LED "tick"?
static volatile bool speaker_tick, led_tick;  // current state
static bool speaker_tick_wanted, led_tick_wanted;  // state wanted by user

// MUX (mutexes used for mutual exclusive access to isr variables)
portMUX_TYPE mux_audio = portMUX_INITIALIZER_UNLOCKED;

// Keep sequences in DRAM so the ISR never dereferences flash when cache is off.
static DRAM_ATTR int alarm_sequence[][4] = {
  {3000000, 1, -1, 400},  // high pitch
  {1000000, 1, -1, 400},  // low pitch
  {0, 0, -1, 0},          // end
};

static DRAM_ATTR int init_sequence[][4] = {
  {0, 0, 0, 0},  // speaker off, led off, end
};

static DRAM_ATTR int melody_sequence[][4] = {
  {int(1174659 * 0.75), 1, -1, 2},  // D
  {0, 0, -1, 2},                    // ---
  {int(1318510 * 0.75), 1, -1, 2},  // E
  {0, 0, -1, 2},                    // ---
  {int(1479978 * 0.75), 1, -1, 2},  // Fis
  {0, 0, -1, 2},                    // ---
  {int(1567982 * 0.75), 1, -1, 4},  // G
  {int(1174659 * 0.75), 1, -1, 2},  // D
  {int(1318510 * 0.75), 1, -1, 2},  // E
  {int(1174659 * 0.75), 1, -1, 4},  // D
  {int(987767 * 0.75), 1, -1, 2},   // H
  {int(1046502 * 0.75), 1, -1, 2},  // C
  {int(987767 * 0.75), 1, -1, 4},   // H
  {int(987767 * 0.75), 0, -1, 4},   // H
  {0, 0, -1, 2},                    // ---
  {0, 0, -1, 0},                    // speaker off, end
};

enum class AudioCommandType { TickHigh, TickLow, PlaySequence };

struct AudioCommand {
  AudioCommandType type;
  const int (*sequence)[4];
  size_t length;
};

static QueueHandle_t audio_command_queue = nullptr;

static void speakerOn(int frequency_mHz, int volume) {
  if (frequency_mHz <= 0)
    return;
  if (volume >= 1) {
    // high volume - MCPWM A/B outputs generate inverted signals
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, MCPWM_DUTY_MODE_1);
  } else {
    // low volume - do MCPWM on A, keep B permanently low
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
    mcpwm_set_signal_low(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B);
  }
  mcpwm_set_frequency(MCPWM_UNIT_0, MCPWM_TIMER_0, frequency_mHz / 1000);
  mcpwm_start(MCPWM_UNIT_0, MCPWM_TIMER_0);
}

static void speakerOff() {
  mcpwm_stop(MCPWM_UNIT_0, MCPWM_TIMER_0);
  mcpwm_set_signal_high(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A);
  mcpwm_set_signal_low(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B);
}

static void audioTask(void * /*param*/) {
  AudioCommand cmd;
  for (;;) {
    if (xQueueReceive(audio_command_queue, &cmd, portMAX_DELAY) != pdTRUE)
      continue;

    switch (cmd.type) {
    case AudioCommandType::TickHigh:
      if (speaker_tick)
        speakerOn(5000000, 1);  // 5 kHz
      if (led_tick)
        digitalWrite(LED_BUILTIN, HIGH);
      vTaskDelay(pdMS_TO_TICKS(4));
      speakerOff();
      if (led_tick)
        digitalWrite(LED_BUILTIN, LOW);
      break;
    case AudioCommandType::TickLow:
      if (speaker_tick)
        speakerOn(1000000, 1);  // 1 kHz
      if (led_tick)
        digitalWrite(LED_BUILTIN, LOW);
      vTaskDelay(pdMS_TO_TICKS(4));
      speakerOff();
      break;
    case AudioCommandType::PlaySequence:
      if (cmd.sequence && cmd.length > 0) {
        for (size_t i = 0; i < cmd.length; i++) {
          int frequency_mHz = cmd.sequence[i][0];
          int volume = cmd.sequence[i][1];
          int led = cmd.sequence[i][2];
          int duration_ms = cmd.sequence[i][3];
          if (frequency_mHz > 0)
            speakerOn(frequency_mHz, volume);
          else if (frequency_mHz == 0)
            speakerOff();
          if (led >= 0)
            digitalWrite(LED_BUILTIN, led ? HIGH : LOW);
          if (duration_ms > 0)
            vTaskDelay(pdMS_TO_TICKS(duration_ms));
        }
        speakerOff();
        digitalWrite(LED_BUILTIN, LOW);
      }
      break;
    }
  }
}


// Audio timer is unused; keep stub ISR to avoid flash access
void IRAM_ATTR isr_audio() {
  // nothing: all audio handled in FreeRTOS task
}

void IRAM_ATTR tick(bool high) {
  // high true: "tick" -> high frequency tick and LED blink
  // high false: "tock" -> lower frequency tock, no LED
  // called from ISR!
  portENTER_CRITICAL_ISR(&mux_audio);
  if (speaker_tick || led_tick) {
    AudioCommand cmd;
    cmd.type = high ? AudioCommandType::TickHigh : AudioCommandType::TickLow;
    cmd.sequence = nullptr;
    cmd.length = 0;
    BaseType_t hpw = pdFALSE;
    if (audio_command_queue)
      xQueueSendFromISR(audio_command_queue, &cmd, &hpw);
  }
  portEXIT_CRITICAL_ISR(&mux_audio);
}

void tick_enable(bool enable) {
  // true -> bring ticking into the state desired by user
  // false -> disable ticking (e.g. when accessing flash)
  if (enable) {
    led_tick = led_tick_wanted;
    speaker_tick = speaker_tick_wanted;
  } else {
    led_tick = false;
    speaker_tick = false;
  }
}

void alarm() {
  // play alarm sound, called from normal code (not ISR)
  if (audio_command_queue) {
    AudioCommand cmd;
    cmd.type = AudioCommandType::PlaySequence;
    cmd.sequence = alarm_sequence;
    cmd.length = sizeof(alarm_sequence) / sizeof(alarm_sequence[0]);
    xQueueSend(audio_command_queue, &cmd, 0);
  }
}

static void play(int *sequence) {
  // legacy wrapper, unused
}

static void play_sequence(const int (*sequence)[4], size_t len) {
  if (audio_command_queue) {
    AudioCommand cmd;
    cmd.type = AudioCommandType::PlaySequence;
    cmd.sequence = sequence;
    cmd.length = len;
    xQueueSend(audio_command_queue, &cmd, 0);
  }
}

void setup_speaker(bool playSound, bool _led_tick, bool _speaker_tick) {
  pinMode(LED_BUILTIN, OUTPUT);

  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, PIN_SPEAKER_OUTPUT_P);
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, PIN_SPEAKER_OUTPUT_N);

  mcpwm_config_t pwm_config;
  pwm_config.frequency = 1000;
  // set duty cycles to 50% (and never modify them again!)
  pwm_config.cmpr_a = 50.0;
  pwm_config.cmpr_b = 50.0;
  pwm_config.duty_mode = MCPWM_DUTY_MODE_0;  // active high duty
  pwm_config.counter_mode = MCPWM_UP_COUNTER;
  mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);

  tick_enable(false);  // no ticking while we play melody / init sound

  if (!audio_command_queue) {
    audio_command_queue = xQueueCreate(8, sizeof(AudioCommand));
    xTaskCreate(audioTask, "audioTask", 4096, NULL, 1, NULL);
  }

  play_sequence(init_sequence, sizeof(init_sequence) / sizeof(init_sequence[0]));

  if (playSound)
    play_sequence(melody_sequence, sizeof(melody_sequence) / sizeof(melody_sequence[0]));

  led_tick_wanted = _led_tick;
  speaker_tick_wanted = _speaker_tick;
  tick_enable(true);
}

// Timer helpers

#define RECHARGE_TIMER 0

hw_timer_t *recharge_timer = NULL;

hw_timer_t *setup_timer(int timer_no, void (*isr)(), int period_us) {
  hw_timer_t *timer = timerBegin(timer_no, 80, true);  // prescaler: 80MHz / 80 == 1MHz
  timerAttachInterrupt(timer, isr, true);  // set ISR
  timerAlarmWrite(timer, period_us, true);  // set alarm after period, do repeat
  timerWrite(timer, 0);
  timerAlarmEnable(timer);
  return timer;
}

void setup_recharge_timer(void (*isr_recharge)(), int period_us) {
  recharge_timer = setup_timer(RECHARGE_TIMER, isr_recharge, period_us);
}
