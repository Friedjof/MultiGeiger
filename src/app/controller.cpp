#include "controller.hpp"

// Measurement interval (default 2.5min) [sec]
static const unsigned long MEASUREMENT_INTERVAL = 90;

// Max time the greeting display will be on. [msec]
static const unsigned long AFTERSTART = 5000;

// In which intervals the OLED display is updated. [msec]
static const unsigned long DISPLAYREFRESH = 10000;

// Minimum amount of GM pulses required to early-update the display.
static const unsigned long MINCOUNTS = 100;

// Target loop duration [ms]
static const unsigned long LOOP_DURATION = 1000;

void MultiGeigerController::begin() {
  isLoraBoard = io.detectLoRa();
  setup_log(DEFAULT_LOG_LEVEL);
  display.begin(isLoraBoard);
  io.setupSwitches(isLoraBoard);
  switches_state = io.readSwitches();  // only read DIP switches once at boot time
  sensors.beginThp();
  wifi.beginWeb(isLoraBoard);
  io.setupSpeaker(playSound, ledTick && switches_state.led_on, speakerTick && switches_state.speaker_on);
  wifi.beginTx(VERSION_STR, ssid, isLoraBoard);
  MqttConfig mqttCfg{
    .enabled = sendToMqtt,
    .host = String(mqttHost),
    .port = mqttPort,
    .useTls = mqttUseTls,
    .username = String(mqttUsername),
    .password = String(mqttPassword),
    .retain = mqttRetain,
    .qos = mqttQos,
    .baseTopic = String(mqttBaseTopic)
  };
  mqtt.begin(mqttCfg, ssid);
  ble.begin(ssid, sendToBle && switches_state.ble_on);
  setup_log_data(SERIAL_DEBUG);
  sensors.beginTube();
  log(DEBUG, "All Setup done");
}

void MultiGeigerController::setupNtp(int wifi_status) {
  static bool clock_configured = false;
  if (clock_configured)
    return;

  if (wifi_status != ST_WIFI_CONNECTED)
    return;

  clock.begin(0);  // 0 == do NTP!
  clock_configured = true;
}

int MultiGeigerController::updateWifiStatus() {
  int st;
  switch (iotWebConf.getState()) {
  case iotwebconf::Connecting:
    st = ST_WIFI_CONNECTING;
    break;
  case iotwebconf::OnLine:
    st = ST_WIFI_CONNECTED;
    break;
  case iotwebconf::ApMode:
    st = ST_WIFI_AP;
    break;
  default:
    st = ST_WIFI_OFF;
    break;
  }
  display.setStatus(STATUS_WIFI, st);
  return st;
}

int MultiGeigerController::updateBleStatus() {  // currently no error detection
  int st;
  if (sendToBle && switches_state.ble_on)
    st = ble.connected() ? ST_BLE_CONNECTED : ST_BLE_CONNECTABLE;
  else
    st = ST_BLE_OFF;
  display.setStatus(STATUS_BLE, st);
  return st;
}

void MultiGeigerController::publish(unsigned long current_ms, unsigned long current_counts, unsigned long gm_count_timestamp, unsigned long current_hv_pulses,
                                    float temperature, float humidity, float pressure) {
  static unsigned long last_timestamp = millis();
  static unsigned long last_counts = 0;
  static unsigned long last_hv_pulses = 0;
  static unsigned long last_count_timestamp = 0;
  static unsigned int accumulated_GMC_counts = 0;
  static unsigned long accumulated_time = 0;
  static float accumulated_Count_Rate = 0.0, accumulated_Dose_Rate = 0.0;

  if (((current_counts - last_counts) >= MINCOUNTS) || ((current_ms - last_timestamp) >= DISPLAYREFRESH)) {
    if ((gm_count_timestamp == 0) && (last_count_timestamp == 0)) {
      return;
    }
    last_timestamp = current_ms;
    int hv_pulses_delta = current_hv_pulses - last_hv_pulses;
    last_hv_pulses = current_hv_pulses;
    int counts = current_counts - last_counts;
    last_counts = current_counts;
    int dt = gm_count_timestamp - last_count_timestamp;
    last_count_timestamp = gm_count_timestamp;

    accumulated_time += dt;
    accumulated_GMC_counts = current_counts;

    float GMC_factor_uSvph = tubes[TUBE_TYPE].cps_to_uSvph;

    float Count_Rate = (dt != 0) ? (float)counts * 1000.0 / (float)dt : 0.0;
    float Dose_Rate = Count_Rate * GMC_factor_uSvph;

    accumulated_Count_Rate = (accumulated_time != 0) ? (float)accumulated_GMC_counts * 1000.0 / (float)accumulated_time : 0.0;
    accumulated_Dose_Rate = accumulated_Count_Rate * GMC_factor_uSvph;

    ble.update((unsigned int)(Count_Rate * 60));
    display.showGmc((unsigned int)(accumulated_time / 1000), (int)(accumulated_Dose_Rate * 1000), (int)(Count_Rate * 60),
                    (showDisplay && switches_state.display_on));
    mqtt.publishLive(Count_Rate, Dose_Rate, counts, dt, hv_pulses_delta,
                     accumulated_GMC_counts, accumulated_time, accumulated_Count_Rate, accumulated_Dose_Rate,
                     temperature, humidity, pressure);

    if (soundLocalAlarm && GMC_factor_uSvph > 0) {
      if (accumulated_Dose_Rate > localAlarmThreshold) {
        log(WARNING, "Local alarm: Accumulated dose of %.3f µSv/h above threshold at %.3f µSv/h", accumulated_Dose_Rate, localAlarmThreshold);
        io.triggerAlarm();
      } else if (Dose_Rate > (accumulated_Dose_Rate * localAlarmFactor)) {
        log(WARNING, "Local alarm: Current dose of %.3f > %d x accumulated dose of %.3f µSv/h", Dose_Rate, localAlarmFactor, accumulated_Dose_Rate);
        io.triggerAlarm();
      }
    }

    if (Serial_Print_Mode == Serial_Logging) {
      log_data(counts, dt, Count_Rate, Dose_Rate, hv_pulses_delta,
               accumulated_GMC_counts, accumulated_time, accumulated_Count_Rate, accumulated_Dose_Rate,
               temperature, humidity, pressure);
    }
  } else {
    static unsigned long boot_timestamp = millis();
    static unsigned long afterStartTime = AFTERSTART;
    if (afterStartTime && ((current_ms - boot_timestamp) >= afterStartTime)) {
      afterStartTime = 0;
      ble.update(0);
      display.showGmc(0, 0, 0, (showDisplay && switches_state.display_on));
    }
  }
}

void MultiGeigerController::oneMinuteLog(unsigned long current_ms, unsigned long current_counts) {
  static unsigned int last_counts = 0;
  static unsigned long last_timestamp = millis();
  int dt = current_ms - last_timestamp;
  if (dt >= 60000) {
    unsigned long counts = current_counts - last_counts;
    unsigned int count_rate = (counts * 60000) / dt;
    if (((((counts * 60000) % dt) * 2) / dt) >= 1) {
      count_rate++;  // Rounding + 0.5
    }
    log_data_one_minute((current_ms / 1000), count_rate, counts);
    last_timestamp = current_ms;
    last_counts = current_counts;
  }
}

void MultiGeigerController::statisticsLog(unsigned long current_counts, unsigned int time_between) {
  static unsigned long last_counts = 0;
  if (current_counts != last_counts) {
    log_data_statistics(time_between);
    last_counts = current_counts;
  }
}

void MultiGeigerController::readThp(unsigned long current_ms) {
  static unsigned long last_timestamp = 0;
  if (!last_timestamp || (current_ms - last_timestamp) >= (MEASUREMENT_INTERVAL * 1000)) {
    last_timestamp = current_ms;
    have_thp = sensors.readThp(temperature, humidity, pressure);
  }
}

void MultiGeigerController::transmit(unsigned long current_ms, unsigned long current_counts, unsigned long gm_count_timestamp, unsigned long current_hv_pulses,
                                     bool have_thp_in, float temperature_in, float humidity_in, float pressure_in, int wifi_status) {
  static unsigned long last_counts = 0;
  static unsigned long last_hv_pulses = 0;
  static unsigned long last_timestamp = millis();
  static unsigned long last_count_timestamp = 0;
  if ((current_ms - last_timestamp) >= (MEASUREMENT_INTERVAL * 1000)) {
    if ((gm_count_timestamp == 0) && (last_count_timestamp == 0)) {
      return;
    }
    last_timestamp = current_ms;
    unsigned long counts = current_counts - last_counts;
    last_counts = current_counts;
    int dt = gm_count_timestamp - last_count_timestamp;
    last_count_timestamp = gm_count_timestamp;
    unsigned int current_cpm = (dt != 0) ? (int)(counts * 60000 / dt) : 0;

    int hv_pulses_delta = current_hv_pulses - last_hv_pulses;
    last_hv_pulses = current_hv_pulses;

    log(DEBUG, "Measured GM: cpm= %d HV=%d", current_cpm, hv_pulses_delta);

    wifi.send(tubes[TUBE_TYPE].type, tubes[TUBE_TYPE].nbr, dt, hv_pulses_delta, counts, current_cpm,
              have_thp_in, temperature_in, humidity_in, pressure_in, wifi_status);
    mqtt.publishMeasurement(tubes[TUBE_TYPE].type, tubes[TUBE_TYPE].nbr, dt, hv_pulses_delta, counts, current_cpm,
                            have_thp_in, temperature_in, humidity_in, pressure_in, wifi_status);
  }
}

void MultiGeigerController::loopOnce() {
  unsigned long current_ms = millis();

  sensors.readTube(gm_counts, gm_count_timestamp, gm_count_time_between);

  readThp(current_ms);

  sensors.readHv(hv_error, hv_pulses);
  display.setStatus(STATUS_HV, hv_error ? ST_HV_ERROR : ST_HV_OK);

  int wifi_status = updateWifiStatus();
  setupNtp(wifi_status);

  updateBleStatus();

  wifi.pollTx();
  wifi.pollWeb();
  mqtt.loop();

  publish(current_ms, gm_counts, gm_count_timestamp, hv_pulses, temperature, humidity, pressure);

  if (Serial_Print_Mode == Serial_One_Minute_Log)
    oneMinuteLog(current_ms, gm_counts);

  if (Serial_Print_Mode == Serial_Statistics_Log)
    statisticsLog(gm_counts, gm_count_time_between);

  transmit(current_ms, gm_counts, gm_count_timestamp, hv_pulses, have_thp, temperature, humidity, pressure, wifi_status);

  long loop_duration = millis() - current_ms;
  iotWebConf.delay((loop_duration < LOOP_DURATION) ? (LOOP_DURATION - loop_duration) : 0);
}

void MultiGeigerController::applyTickSettings(bool ledTick, bool speakerTick) {
  // Apply tick settings respecting DIP switch state
  io.updateTickSettings(ledTick && switches_state.led_on, speakerTick && switches_state.speaker_on);
}

void MultiGeigerController::applyDisplaySetting(bool showDisplay) {
  // Apply display setting respecting DIP switch state
  display.applyDisplaySetting(showDisplay && switches_state.display_on);
}
