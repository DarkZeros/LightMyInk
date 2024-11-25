#include "light.h"
#include "hardware.h"
#include "power.h"

#include "driver/gpio.h"

#include "esp32-hal-log.h"

#include "Arduino.h"

namespace {
  RTC_DATA_ATTR bool kPrev{false};
} // namespace

#include "soc/rtc_periph.h"
#include "rom/gpio.h"
#include "deep_sleep_utils.h"

void Light::onFor(uint32_t ms) {
  on();
  delay(ms);
  off();
}

bool Light::toggle() {
  set(!kPrev);
  return kPrev;
}

void Light::set(bool high) {
  // Caches previous values
  if (kPrev == high)
    return;

  // Not initialized in DeepSleep
  // const rtc_io_desc_t& desc = rtc_io_desc[rtc_io_num_map[HW::kLightPin]];
  const rtc_io_desc_t desc = {RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_MUX_SEL_M, RTC_IO_PDAC1_FUN_SEL_S, RTC_IO_PDAC1_FUN_IE_M, RTC_IO_PDAC1_RUE_M, RTC_IO_PDAC1_RDE_M, RTC_IO_PDAC1_SLP_SEL_M, RTC_IO_PDAC1_SLP_IE_M, 0, RTC_IO_PDAC1_HOLD_M, RTC_CNTL_PDAC1_HOLD_FORCE_M, RTC_IO_PDAC1_DRV_V, RTC_IO_PDAC1_DRV_S, RTCIO_CHANNEL_6_GPIO_NUM};
  // Hold disable
  REG_CLR_BIT(RTC_CNTL_HOLD_FORCE_REG, desc.hold_force);
  REG_CLR_BIT(desc.reg, desc.hold);
  // Deep sleep hold disable
  CLEAR_PERI_REG_MASK(RTC_CNTL_DIG_ISO_REG, RTC_CNTL_DG_PAD_AUTOHOLD_EN_M);

  GPIO_MODE_OUTPUT(25);
  GPIO_OUTPUT_SET(HW::kLightPin, high);

  // LED requires high power while it is on, locking it
  if (high) {
    Power::lock();
  } else {
    Power::unlock();
  }

  // Hold enable
  REG_SET_BIT(RTC_CNTL_HOLD_FORCE_REG, desc.hold_force);
  REG_SET_BIT(desc.reg, desc.hold);

  // Deep sleep hold enable
  CLEAR_PERI_REG_MASK(RTC_CNTL_DIG_ISO_REG, RTC_CNTL_DG_PAD_FORCE_UNHOLD);
  SET_PERI_REG_MASK(RTC_CNTL_DIG_ISO_REG, RTC_CNTL_DG_PAD_AUTOHOLD_EN_M);

  kPrev = high;
}
