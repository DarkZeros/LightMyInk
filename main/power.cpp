#include "power.h"
#include "hardware.h"

#include "esp_attr.h"
#include "driver/gpio.h"

RTC_DATA_ATTR bool kSet{false};
RTC_DATA_ATTR bool kPrevVoltage{false};

void Power::setVoltage(bool high) {
  constexpr const gpio_config_t kConf = {
    .pin_bit_mask = (1ULL<<HW::kVoltageSelectPin),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
  };
  // Caches previous values
  if (kSet && kPrevVoltage == high)
    return;

  //Configure GPIO with the given settings
  gpio_config(&kConf);

  //Disable previous holds
  gpio_hold_dis((gpio_num_t)HW::kVoltageSelectPin);
  gpio_deep_sleep_hold_dis();

  // Set new level
  gpio_set_level((gpio_num_t)HW::kVoltageSelectPin, high ? 1 : 0);
  
  // Set the holds for sleep modes
  gpio_hold_en((gpio_num_t)HW::kVoltageSelectPin);
  gpio_deep_sleep_hold_en();

  // ESP_LOGI("Power", "Changed to %d", high);
  kSet = true;
  kPrevVoltage = high;
}