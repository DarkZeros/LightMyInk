
#pragma once

#include "esp_attr.h"

#include <atomic>

/* Helper class to handle light on/off and keeping it in deepsleep
 */
struct Light {
    static void RTC_IRAM_ATTR on() { set(true); }
    static void RTC_IRAM_ATTR off() { set(false); }
    static bool RTC_IRAM_ATTR toggle();
    static void RTC_IRAM_ATTR set(bool high);
    static void onFor(uint32_t us);
};