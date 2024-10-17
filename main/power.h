
#pragma once

#define DEEP_SLEEP_SAFE_VOLTAGE

#ifdef DEEP_SLEEP_SAFE_VOLTAGE
    #include "esp_attr.h"
    #define POWER_IRAM RTC_IRAM_ATTR
#else
    #define POWER_IRAM
#endif

/* Helper class to handle extra power needed for WiFi operations
 */
struct Power {
    static void POWER_IRAM high() { setVoltage(true); }
    static void POWER_IRAM low() { setVoltage(false); }
    static void POWER_IRAM setVoltage(bool high);
};