
#pragma once

#include "esp_attr.h"

#include <atomic>

/* Helper class to handle extra power needed for WiFi/Display operations
 */
struct Power {
    static void RTC_IRAM_ATTR high() { set(true); }
    static void RTC_IRAM_ATTR low() { set(false); }
    static void RTC_IRAM_ATTR set(bool high);

    static void RTC_IRAM_ATTR lock();
    static void RTC_IRAM_ATTR unlock();
};