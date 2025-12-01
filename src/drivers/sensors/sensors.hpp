// Sensor-related code: GM tube handling and THP sensor support.

#pragma once

#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BME680.h>

#include "core/core.hpp"
#include "drivers/io/io.hpp"
#include "config/config.hpp"

typedef struct {
  const char *type;          // type string for sensor.community
  const char nbr;            // number to be sent by LoRa
  const float cps_to_uSvph;  // factor to convert counts per second to µSievert per hour
} TUBETYPE;

extern TUBETYPE tubes[];

void setup_tube(void);
void read_GMC(unsigned long *counts, unsigned long *timestamp, unsigned int *between);
void read_hv(bool *hv_error, unsigned long *pulses);

bool setup_thp_sensor(void);
bool read_thp_sensor(float *temperature, float *humidity, float *pressure);

// Thin OO wrapper for sensor handling.
class Sensors {
public:
  bool beginThp() { return setup_thp_sensor(); }
  bool readThp(float &t, float &h, float &p) { return read_thp_sensor(&t, &h, &p); }

  void beginTube() { setup_tube(); }
  void readTube(unsigned long &counts, unsigned long &timestamp, unsigned int &between) { read_GMC(&counts, &timestamp, &between); }
  void readHv(bool &hvError, unsigned long &pulses) { read_hv(&hvError, &pulses); }
};
