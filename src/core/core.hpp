// Core utilities: logging, data logging helpers, misc utils, version string.

#pragma once

#include <Arduino.h>
#include <sys/time.h>
#include <stdarg.h>
#include "drivers/clock/clock.hpp"

// log levels
#define DEBUG 0
#define INFO 1
#define WARNING 2
#define ERROR 3
#define CRITICAL 4
#define NOLOG 999  // only to set log_level, so log() never creates output

// Values for Serial_Print_Mode to configure Serial (USB) output mode.
#define Serial_None 0            // No Serial output
#define Serial_Debug 1           // Only debug and error messages
#define Serial_Logging 2         // Log measurements as a table
#define Serial_One_Minute_Log 3  // One Minute logging
#define Serial_Statistics_Log 4  // Logs time [us] between two events

extern int Serial_Print_Mode;

class CoreServices {
public:
  static void setupLogger(int level);
  static void logMessage(int level, const char *format, ...);
  static void logMessageVa(int level, const char *format, va_list args);

  static void setupDataLogging(int mode);
  static void logData(int GMC_counts, int time_difference, float Count_Rate, float Dose_Rate, int HV_pulse_count,
                      int accumulated_GMC_counts, int accumulated_time, float accumulated_Count_Rate, float accumulated_Dose_Rate,
                      float t, float h, float p);
  static void logDataOneMinute(int time_s, int cpm, int counts);
  static void logDataStatistics(int count_time_between);

  static int hex2data(unsigned char *data, const char *hexstring, unsigned int len);
  static void reverseByteArray(unsigned char *data, int len);
};

// Keep legacy free-function API for existing call sites.
void log(int level, const char *format, ...);
void setup_log(int level);
void setup_log_data(int mode);
void log_data(int GMC_counts, int time_difference, float Count_Rate, float Dose_Rate, int HV_pulse_count,
              int accumulated_GMC_counts, int accumulated_time, float accumulated_Count_Rate, float accumulated_Dose_Rate,
              float t, float h, float p);
void log_data_one_minute(int time_s, int cpm, int counts);
void log_data_statistics(int count_time_between);
int hex2data(unsigned char *data, const char *hexstring, unsigned int len);
void reverseByteArray(unsigned char *data, int len);

// Release version
// To fit in 16bit for lora version number, we have some limits here:
// major = 0..15, minor = 0..255, patch = 0..15.
//
// vvv--- do not edit here, this is managed by bump2version! ---vvv
#define VERSION_STR "V1.17.0-dev"
// ^^^--- do not edit here, this is managed by bump2version! ---^^^
