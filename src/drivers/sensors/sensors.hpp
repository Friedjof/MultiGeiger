/**
 * @file sensors.hpp
 * @brief Sensor interface for GM tube and environmental sensors
 *
 * Provides interfaces for:
 * - Geiger-Müller tube radiation detection
 * - Temperature, Humidity, Pressure (THP) sensors (BME280/BME680)
 * - High voltage monitoring
 */

#pragma once

#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BME680.h>

#include "core/core.hpp"
#include "drivers/io/io.hpp"
#include "config/config.hpp"

/**
 * @struct TUBETYPE
 * @brief Geiger-Müller tube type definition
 */
typedef struct {
  const char *type;          ///< Tube type string for sensor.community API
  const char nbr;            ///< Tube type number for LoRa transmission
  const float cps_to_uSvph;  ///< Conversion factor: counts/sec to µSievert/hour
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
