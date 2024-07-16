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

// counter value, stored in RTC memory
RTC_DATA_ATTR uint32_t s_count = 0;
RTC_DATA_ATTR const uint32_t s_max_count = 10;

// wakeup_cause stored in RTC memory
RTC_DATA_ATTR uint32_t wakeup_cause;

// wakeup_time from CPU start to wake stub
RTC_DATA_ATTR uint32_t wakeup_time;


void RTC_IRAM_ATTR delayMicroseconds(uint32_t us) {
  const auto ticks = esp_rom_get_cpu_ticks_per_us();
  auto m = esp_cpu_get_cycle_count();
  auto e = (m + us * ticks);
  while (esp_cpu_get_cycle_count() < e) {
    asm volatile("nop");
  }
}
void RTC_IRAM_ATTR delayNanoseconds(uint32_t ns) {
  const auto ticks = esp_rom_get_cpu_ticks_per_us();
  auto m = esp_cpu_get_cycle_count();
  auto e = (m + ns * ticks / 1000);
  while (esp_cpu_get_cycle_count() < e) {
    asm volatile("nop");
  }
}

void RTC_IRAM_ATTR _transfer(uint8_t value)
{
  for (auto i=0; i<8; i++)
  {
    // Set value
    GPIO_OUTPUT_SET(HW::DisplayPin::Mosi, (value >> (7-i)) & 0b1);
    // Cycle Clock
    delayMicroseconds(1);
    GPIO_OUTPUT_SET(HW::DisplayPin::Sck, 1);
    //delayNanoseconds(1000/20);
    delayMicroseconds(1);
    GPIO_OUTPUT_SET(HW::DisplayPin::Sck, 0);
    //delayNanoseconds(1000/20);
  }
}

void RTC_IRAM_ATTR _transferCommand(uint8_t value)
{
    GPIO_OUTPUT_SET(HW::DisplayPin::Dc, 0);
    _transfer(value);
    GPIO_OUTPUT_SET(HW::DisplayPin::Dc, 1);
}

#include "SPI.h"

// wake up stub function stored in RTC memory
void RTC_IRAM_ATTR wake_stub_example(void)
{
    // Get wakeup time.
    wakeup_time = esp_cpu_get_cycle_count() / esp_rom_get_cpu_ticks_per_us();
    // Get wakeup cause.
    wakeup_cause = esp_wake_stub_get_wakeup_cause();
    // Increment the counter.
    s_count++;
    // // Print the counter value and wakeup cause.
    // ESP_RTC_LOGE("wake stub: wakeup count is %d, wakeup cause is %d, wakeup cost %ld us", s_count, wakeup_cause, wakeup_time);

    if (s_count >= s_max_count) {
        // Reset s_count
        s_count = 0;

        // Set the default wake stub.
        // There is a default version of this function provided in esp-idf.
        esp_default_wake_deep_sleep();

        // Return from the wake stub function to continue
        // booting the firmware.
        return;
    }

    // Reset display to wake it up
    GPIO_OUTPUT_SET(HW::DisplayPin::Res, 0);
    // pinMode(HW::DisplayPin::Res, OUTPUT);
    delayMicroseconds(1000);
    //pinMode(HW::DisplayPin::Res, INPUT_PULLUP);
    GPIO_OUTPUT_SET(HW::DisplayPin::Res, 1);    

    // Set the CS low to select our Display
    GPIO_OUTPUT_SET(HW::DisplayPin::Cs, 0);

    // Copy the new data to the display buffer?
    _transferCommand(0x3C); // BorderWavefrom
    static RTC_DATA_ATTR bool dark = true;
    _transfer(dark ? 0x02 : 0x05);
    dark = !dark;

    // Refresh the display (the buffer was already set before)!
    _transferCommand(0x20);
    // Wait busy, or just sleep 400ms
    delayMicroseconds(400'000);
    _transferCommand(0x10); // change deep sleep mode
    _transfer(0b01);  // mode 1 (RAM reading allowed)
    // Finish! go back to deep sleep


    // // Set wakeup time in stub, if need to check GPIOs or read some sensor periodically in the stub.
    esp_wake_stub_set_wakeup_time(1 * 1'000'000);

    // // Print status.
    // ESP_RTC_LOGE("wake stub: going to deep sleep");

    // Set stub entry, then going to deep sleep again.
    esp_wake_stub_sleep(&wake_stub_example);
}