#include "core.hpp"

// Logging

// the GEIGER: prefix is is to easily differentiate our output from other esp32 output (e.g. wifi messages)
#define LOG_PREFIX_FORMAT "GEIGER: %s "
#define LOG_PREFIX_LEN (7+1+19+1)  // chars, without the terminating \0

static int log_level = NOLOG;  // messages at level >= log_level will be output

int Serial_Print_Mode;

static const char *Serial_Logging_Name = "Simple Multi-Geiger";
static const char *dashes = "------------------------------------------------------------------------------------------------------------------------";

static const char *Serial_Logging_Header = "     %10s %15s %10s %9s %9s %8s %9s %9s %9s %5s %5s %6s";
static const char *Serial_Logging_Body = "DATA %10d %15d %10f %9f %9d %8d %9d %9f %9f %5.1f %5.1f %6.0f";
static const char *Serial_One_Minute_Log_Header = "     %4s %10s %29s";
static const char *Serial_One_Minute_Log_Body = "DATA %4d %10d %29d";

void CoreServices::setupLogger(int level) {
  Serial.begin(115200);
  while (!Serial) {};
  CoreServices::logMessage(NOLOG, "Logging initialized at level %d.", level);  // this will always be output
  log_level = level;
}

void CoreServices::logMessage(int level, const char *format, ...) {
  va_list args;
  va_start(args, format);
  CoreServices::logMessageVa(level, format, args);
  va_end(args);
}

void CoreServices::logMessageVa(int level, const char *format, va_list args) {
  if (level < log_level)
    return;

  va_list args_copy;
  va_copy(args_copy, args);
  int needed = vsnprintf(NULL, 0, format, args_copy);
  va_end(args_copy);

  char buf[needed + 1 + LOG_PREFIX_LEN];
  sprintf(buf, LOG_PREFIX_FORMAT, utctime());

  va_list args_copy2;
  va_copy(args_copy2, args);
  vsprintf(buf + LOG_PREFIX_LEN, format, args_copy2);
  va_end(args_copy2);
  Serial.println(buf);
}

void CoreServices::setupDataLogging(int mode) {
  Serial_Print_Mode = mode;

  bool data_log_enabled = (Serial_Print_Mode == Serial_Logging) || (Serial_Print_Mode == Serial_One_Minute_Log) || (Serial_Print_Mode == Serial_Statistics_Log);
  if (data_log_enabled) {
    CoreServices::logMessage(INFO, dashes);
    CoreServices::logMessage(INFO, "%s, Version %s", Serial_Logging_Name, VERSION_STR);
    CoreServices::logMessage(INFO, dashes);
  }
}

void CoreServices::logData(int GMC_counts, int time_difference, float Count_Rate, float Dose_Rate, int HV_pulse_count,
                           int accumulated_GMC_counts, int accumulated_time, float accumulated_Count_Rate, float accumulated_Dose_Rate,
                           float t, float h, float p) {
  static int counter = 0;
  if (counter++ % 20 == 0) {
    CoreServices::logMessage(INFO, Serial_Logging_Header,
                             "GMC_counts", "Time_difference", "Count_Rate", "Dose_Rate", "HV Pulses", "Accu_GMC", "Accu_Time", "Accu_Rate", "Accu_Dose", "Temp", "Humi", "Press");
    CoreServices::logMessage(INFO, Serial_Logging_Header,
                             "[Counts]",   "[ms]",            "[cps]",      "[uSv/h]",   "[-]",       "[Counts]", "[ms]",      "[cps]",     "[uSv/h]",   "[C]",  "[%]",  "[hPa]");
    CoreServices::logMessage(INFO, dashes);
  }
  CoreServices::logMessage(INFO, Serial_Logging_Body,
                           GMC_counts, time_difference, Count_Rate, Dose_Rate, HV_pulse_count,
                           accumulated_GMC_counts, accumulated_time, accumulated_Count_Rate, accumulated_Dose_Rate,
                           t, h, p);
}

void CoreServices::logDataOneMinute(int time_s, int cpm, int counts) {
  static int counter = 0;
  if (counter++ % 20 == 0) {
    CoreServices::logMessage(INFO, Serial_One_Minute_Log_Header,
                             "Time", "Count_Rate", "Counts");
    CoreServices::logMessage(INFO, Serial_One_Minute_Log_Header,
                             "[s]",  "[cpm]",      "[Counts per last measurement]");
    CoreServices::logMessage(INFO, dashes);
  }
  CoreServices::logMessage(INFO, Serial_One_Minute_Log_Body,
                           time_s, cpm, counts);
}

void CoreServices::logDataStatistics(int count_time_between) {
  static int counter = 0;
  if (counter++ % 20 == 0) {
    CoreServices::logMessage(INFO, "Time between two impacts");
    CoreServices::logMessage(INFO, "[usec]");
    CoreServices::logMessage(INFO, dashes);
  }
  CoreServices::logMessage(INFO, "%d", count_time_between);
}

int CoreServices::hex2data(unsigned char *data, const char *hexstring, unsigned int len) {
  const char *pos = hexstring;
  char *endptr;
  size_t count = 0;
  if ((hexstring[0] == '\0') || (strlen(hexstring) % 2)) {
    return -1;
  }
  for (count = 0; count < len; count++) {
    char buf[5] = {'0', 'x', pos[0], pos[1], 0};
    data[count] = strtol(buf, &endptr, 0);
    pos += 2 * sizeof(char);
    if (endptr[0] != '\0') {
      return -1;
    }
  }
  return 0;
}

void CoreServices::reverseByteArray(unsigned char *data, int len) {
  char temp;
  for (int i = 0; i < len / 2; i++) {
    temp = data[i];
    data[i] = data[len - i - 1];
    data[len - i - 1] = temp;
  }
}

// Legacy free-function wrappers
void log(int level, const char *format, ...) {
  va_list args;
  va_start(args, format);
  CoreServices::logMessageVa(level, format, args);
  va_end(args);
}

void setup_log(int level) {
  CoreServices::setupLogger(level);
}

void setup_log_data(int mode) {
  CoreServices::setupDataLogging(mode);
}

void log_data(int GMC_counts, int time_difference, float Count_Rate, float Dose_Rate, int HV_pulse_count,
              int accumulated_GMC_counts, int accumulated_time, float accumulated_Count_Rate, float accumulated_Dose_Rate,
              float t, float h, float p) {
  CoreServices::logData(GMC_counts, time_difference, Count_Rate, Dose_Rate, HV_pulse_count,
                        accumulated_GMC_counts, accumulated_time, accumulated_Count_Rate, accumulated_Dose_Rate,
                        t, h, p);
}

void log_data_one_minute(int time_s, int cpm, int counts) {
  CoreServices::logDataOneMinute(time_s, cpm, counts);
}

void log_data_statistics(int count_time_between) {
  CoreServices::logDataStatistics(count_time_between);
}

int hex2data(unsigned char *data, const char *hexstring, unsigned int len) {
  return CoreServices::hex2data(data, hexstring, len);
}

void reverseByteArray(unsigned char *data, int len) {
  CoreServices::reverseByteArray(data, len);
}
