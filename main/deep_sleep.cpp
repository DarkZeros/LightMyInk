/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <inttypes.h>
#include "esp_sleep.h"
#include "esp_cpu.h"
#include "esp_rom_sys.h"
#include "esp_wake_stub.h"
#include "sdkconfig.h"

#include "rom/gpio.h"

#include "hardware.h"

/*
 * Deep sleep wake stub function is a piece of code that will be loaded into 'RTC Fast Memory'.
 * The first way is to use the RTC_IRAM_ATTR attribute to place a function into RTC memory,
 * The second way is to place the function into any source file whose name starts with rtc_wake_stub.
 * Files names rtc_wake_stub* have their contents automatically put into RTC memory by the linker.
 *
 * First, call esp_set_deep_sleep_wake_stub to set the wake stub function as the RTC stub entry,
 * The wake stub function runs immediately as soon as the chip wakes up - before any normal
 * initialisation, bootloader, or ESP-IDF code has run. After the wake stub runs, the SoC
 * can go back to sleep or continue to start ESP-IDF normally.
 *
 * Wake stub code must be carefully written, there are some rules for wake stub:
 * 1) The wake stub code can only access data loaded in RTC memory.
 * 2) The wake stub code can only call functions implemented in ROM or loaded into RTC Fast Memory.
 * 3) RTC memory must include any read-only data (.rodata) used by the wake stub.
 */

#include "uspi.h"
#include "deep_sleep.h"

RTC_DATA_ATTR DeepSleepState kDSState;

void RTC_IRAM_ATTR turnOffGpio() {
  using namespace HW::DisplayPin;
  for (auto& pin : std::array{Cs, Dc, Res, Busy, Mosi, Sck})
    GPIO_DIS_OUTPUT(pin);
}

// wake up stub function stored in RTC memory
void RTC_IRAM_ATTR wake_stub_example(void)
{
  if (kDSState.displayBusy) {
    kDSState.displayBusy = false;

    uSpi::init();

    // Wait until display busy goes off & measure difference
    GPIO_INPUT_ENABLE(19); // TODO: Make it using the variable HW::DisplayPin::Busy
    auto ticks = esp_cpu_get_cycle_count();
    while(GPIO_INPUT_GET(19) != 0) {
      asm volatile("nop");
    }
    // FIXME: Empirically found it is 1/3 of time reported,
    //  it reports 13ticks/us, but runs at 40Mhz likely (40ticks/us)
    const auto rate = esp_rom_get_cpu_ticks_per_us() * 3;
    const auto us = (esp_cpu_get_cycle_count() - ticks) / rate;
    // Adjust up or down as needed, we want to wait for display <500us
    if (us > 500)
      kDSState.updateWait += us-500;
    else if (us == 0) {
      kDSState.updateWait -= 200;
    }

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
    if (kDSState.minutes >= 0)
      esp_wake_stub_set_wakeup_time(kDSState.stepSize * 60'000'000 - kDSState.updateWait);
    else
      esp_wake_stub_set_wakeup_time((kDSState.stepSize + kDSState.minutes) * 60'000'000 - kDSState.updateWait);

    // Set stub entry, then going to deep sleep again.
    esp_wake_stub_sleep(&wake_stub_example);
    esp_deep_sleep_start();
  }

  // Check if we should just do normal wakeup
  // If it is not a timer wakeup, return to handle on the full mode
  if (esp_wake_stub_get_wakeup_cause() != 8 || kDSState.minutes <= 0) {
    // kDSState.wakeCause = esp_wake_stub_get_wakeup_cause();
    // 8 for timer // 256 for touch
    esp_default_wake_deep_sleep();
    return;
  }

  // Reset display to wake it up
  GPIO_INPUT_DISABLE(9); // TODO: Make it using the variable HW::DisplayPin::Res
  GPIO_OUTPUT_SET(HW::DisplayPin::Res, 0);
  uSpi::dMicroseconds(1000);
  GPIO_OUTPUT_SET(HW::DisplayPin::Res, 1);

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

  // Set to wake up again when the Display has finished
  // THIS DOES NOT WORK, WHY?? I had to implement a hardcoded sleep
  // gpio_pin_wakeup_enable(HW::DisplayPin::Busy, GPIO_PIN_INTR_LOLEVEL);
  // esp_sleep_enable_gpio_wakeup();

  // Set wakeup timer when we guess display will finish refreshing, to put it to sleep
  esp_wake_stub_set_wakeup_time(kDSState.updateWait);

  // Set stub entry, then going to deep sleep again.
  esp_wake_stub_sleep(&wake_stub_example);
  esp_deep_sleep_start();
}