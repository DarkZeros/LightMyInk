#include "power.h"
#include "hardware.h"

#include "esp_attr.h"
#include "driver/gpio.h"

#include "esp32-hal-log.h"

namespace {
  RTC_DATA_ATTR bool kSet{false};
  RTC_DATA_ATTR bool kPrevVoltage{false};

  void setupPin() {
    if (kSet)
      return;

    constexpr const gpio_config_t kConf = {
      .pin_bit_mask = (1ULL<<HW::kVoltageSelectPin),
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
    };
    //Configure GPIO with the given settings
    gpio_config(&kConf);
  }
} // namespace

#ifndef DEEP_SLEEP_SAFE_VOLTAGE

void Power::setVoltage(bool high) {
  // The value changes based on the board! Old boards have the values fliped
  if (HW::kRevision >= 2)
    high = !high;

  // Caches previous values
  if (kSet && kPrevVoltage == high)
    return;

  //Disable previous holds
  gpio_hold_dis((gpio_num_t)HW::kVoltageSelectPin);
  gpio_deep_sleep_hold_dis();

  //Configure GPIO with the given settings
  setupPin();

  // Set new level
  gpio_set_level((gpio_num_t)HW::kVoltageSelectPin, high ? 1 : 0);
  
  // Set the holds for sleep modes
  gpio_hold_en((gpio_num_t)HW::kVoltageSelectPin);
  gpio_deep_sleep_hold_en();

  // ESP_LOGE("Power", "Changed to %d", high);
  kSet = true;
  kPrevVoltage = high;
}
#else

#include "soc/rtc_periph.h"
#include "rom/gpio.h"
#include "deep_sleep_utils.h"

void RTC_IRAM_ATTR Power::setVoltage(bool high) {
  // The value changes based on the board! Old boards have the values fliped
  if (HW::kRevision >= 2)
    high = !high;

  // Caches previous values
  if (kSet && kPrevVoltage == high)
    return;

  // Not initialized in DeepSleep
  // const rtc_io_desc_t& desc = rtc_io_desc[rtc_io_num_map[HW::kVoltageSelectPin]];
  const rtc_io_desc_t desc = {RTC_IO_TOUCH_PAD4_REG, RTC_IO_TOUCH_PAD4_MUX_SEL_M, RTC_IO_TOUCH_PAD4_FUN_SEL_S, RTC_IO_TOUCH_PAD4_FUN_IE_M, RTC_IO_TOUCH_PAD4_RUE_M, RTC_IO_TOUCH_PAD4_RDE_M, RTC_IO_TOUCH_PAD4_SLP_SEL_M, RTC_IO_TOUCH_PAD4_SLP_IE_M, 0, RTC_IO_TOUCH_PAD4_HOLD_M, RTC_CNTL_TOUCH_PAD4_HOLD_FORCE_M, RTC_IO_TOUCH_PAD4_DRV_V, RTC_IO_TOUCH_PAD4_DRV_S, RTCIO_CHANNEL_14_GPIO_NUM};

  // Hold disable
  REG_CLR_BIT(RTC_CNTL_HOLD_FORCE_REG, desc.hold_force);
  REG_CLR_BIT(desc.reg, desc.hold);
  // Deep sleep hold disable
  CLEAR_PERI_REG_MASK(RTC_CNTL_DIG_ISO_REG, RTC_CNTL_DG_PAD_AUTOHOLD_EN_M);

  // This should only have been called once on startup
  setupPin();
  GPIO_MODE_OUTPUT(13);
  GPIO_OUTPUT_SET(HW::kVoltageSelectPin, high);

  // Hold enable
  REG_SET_BIT(RTC_CNTL_HOLD_FORCE_REG, desc.hold_force);
  REG_SET_BIT(desc.reg, desc.hold);

  // Deep sleep hold enable
  CLEAR_PERI_REG_MASK(RTC_CNTL_DIG_ISO_REG, RTC_CNTL_DG_PAD_FORCE_UNHOLD);
  SET_PERI_REG_MASK(RTC_CNTL_DIG_ISO_REG, RTC_CNTL_DG_PAD_AUTOHOLD_EN_M);

  kSet = true;
  kPrevVoltage = high;
}

#endif