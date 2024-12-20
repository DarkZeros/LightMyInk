/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <inttypes.h>
#include "esp_sleep.h"
#include "esp_cpu.h"
#include "rom/ets_sys.h"
#include "rom/gpio.h"
#include "esp_rom_sys.h"
#include "esp_wake_stub.h"
#include "sdkconfig.h"

// For touch detection and Light on/off
#include "hal/touch_sensor_ll.h"
// For WDT feeding
#include "hal/wdt_hal.h"

#include "hardware.h"
#include "power.h"
#include "light.h"
#include "uspi.h"
#include "deep_sleep.h"

#include "esp_attr.h"
#include "soc/rtc_periph.h"
#include "rom/gpio.h"

RTC_DATA_ATTR DeepSleepState kDSState;

void RTC_IRAM_ATTR turnOffGpio() {
  using namespace HW::DisplayPin;
  for (auto& pin : std::array{Cs, Dc, Res, Busy, Mosi, Sck}) {
    GPIO_DIS_OUTPUT(pin);
  }
}

void RTC_IRAM_ATTR feed_wdt() {
  // More than 500ms it is better to reset the MWDT0
  TIMERG0.wdtwprotect.wdt_wkey = TIMG_WDT_WKEY_VALUE;
  TIMERG0.wdtfeed.wdt_feed = 1;
  TIMERG0.wdtwprotect.wdt_wkey = 0;
}

void RTC_IRAM_ATTR microSleep(uint32_t micros) {
  constexpr auto step = 400'000;
  feed_wdt();
  while (micros > step) {
    micros -= step;
    esp_rom_delay_us(step);
    feed_wdt();
  }
  esp_rom_delay_us(micros);
  feed_wdt();
}

// wake up stub function stored in RTC memory
void RTC_IRAM_ATTR wake_stub_example(void)
{
  // This sets up the delay to work properly
  ets_update_cpu_frequency_rom(ets_get_detected_xtal_freq() / 1'000'000);

  // If we were waiting for a display finish, we need to complete it first
  if (kDSState.displayBusy) {
    kDSState.displayBusy = false;

    // Go back to low power mode
    Power::unlock();

    uSpi::init();

    // Wait until display busy goes off
    GPIO_MODE_INPUT(19); // TODO: Make it using the variable HW::DisplayPin::Busy
    while(GPIO_INPUT_GET(19) != 0) {
      microSleep(displayWait);
      kDSState.updateWait += displayWait;
      kDSState.updateWaitReduceScale = 0; // Reset it
    }
    // Reduce it a bit every iteration
    kDSState.updateWait -= displayWaitReduce << ++kDSState.updateWaitReduceScale;

    if (kDSState.redrawDec) {
      kDSState.redrawDec = false;
      const auto& dec = kSettings.mWatchface.mCache.mDecimal;
      uSpi::writeArea(dec.data + dec.coord.size()*kSettings.mWatchface.mLastDraw.mMinuteD, dec.coord.x, dec.coord.y, dec.coord.w, dec.coord.h);
    }

    // Set display to sleep and go to sleep
    uSpi::hibernate();
    turnOffGpio();

    // Guess the amount to sleep until next one and advance counters
    kDSState.currentMinutes += kDSState.stepSize;
    kDSState.minutes -= kDSState.stepSize;
    auto minutes = kDSState.stepSize + (kDSState.minutes < 0 ? kDSState.minutes : 0);
    esp_wake_stub_set_wakeup_time(minutes * 60'000'000 - kDSState.updateWait);

    // Set stub entry, then going to deep sleep again.
    esp_wake_stub_sleep(&wake_stub_example);
  }

  // Light on press button?
  // 8 for timer // 256 for touch
  if (esp_wake_stub_get_wakeup_cause() == 256) {
    uint32_t mask;
    touch_ll_read_trigger_status_mask(&mask);

    // 4 = MENU, 32 = BACK/LIGHT, 64 = DOWN, 1 = UP
    if (mask == 32){
      touch_ll_clear_trigger_status_mask(); // This will consume the touch

      // LED is powered from VDD, and we need to rise it to 3.3V,
      // because 1.9V is too low for the LED to light up
      Light::toggle();

      // Go back to sleep, don´t touch the timer, if the user enters menu, then light will stay on
      esp_wake_stub_sleep(&wake_stub_example);
    }
    // Wake up, touch needs to handle by the Main code
    esp_default_wake_deep_sleep();
    return;
  }

  // Check if we should just do normal wakeup
  // If it is not a timer wakeup, return to handle on the Main code
  // 8 for timer // 256 for touch
  if (esp_wake_stub_get_wakeup_cause() != 8 || kDSState.minutes <= 0) {
    esp_default_wake_deep_sleep();
    return;
  }

  // Turn off the light if it is on when we do a normal update
  Light::off();

  // Reset display to wake it up
  GPIO_MODE_OUTPUT(9); // TODO: Make it using the variable HW::DisplayPin::Res
  GPIO_OUTPUT_SET(HW::DisplayPin::Res, 0);
  esp_rom_delay_us(1'000);
  GPIO_OUTPUT_SET(HW::DisplayPin::Res, 1);

  // Turn on high power mode since it makes display use less power (not sure why)
  // It will take around 125us * 0.1V, for 1.5V = 1.6ms
  Power::lock();

  // Calculate the areas to update based on time and watchface states
  const auto u = kDSState.currentMinutes % 10;
  const auto d = kDSState.currentMinutes / 10;

  auto& last = kSettings.mWatchface.mLastDraw;

  uSpi::init();
  if (u != last.mMinuteU[0]) {
    // Write minute U
    const auto& uni = kSettings.mWatchface.mCache.mUnits;
    uSpi::writeArea(uni.data + uni.coord.size()*u, uni.coord.x, uni.coord.y, uni.coord.w, uni.coord.h);
    last.mMinuteU[0] = last.mMinuteU[1];
    last.mMinuteU[1] = u;
  }
  if (d != last.mMinuteD) {
    // Write minute D + repeat it in the display hibernate
    const auto& dec = kSettings.mWatchface.mCache.mDecimal;
    uSpi::writeArea(dec.data + dec.coord.size()*d, dec.coord.x, dec.coord.y, dec.coord.w, dec.coord.h);
    last.mMinuteD = d;
    kDSState.redrawDec = true;
  }
  uSpi::refresh();
  turnOffGpio();
  kDSState.displayBusy = true;

  // Set wakeup timer when we guess display will finish refreshing, to put display to hibernation
  esp_wake_stub_set_wakeup_time(kDSState.updateWait);

  // Set stub entry, then going to deep sleep again.
  esp_wake_stub_sleep(&wake_stub_example);
}