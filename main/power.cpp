#include "power.h"
#include "hardware.h"

#include "driver/gpio.h"

#include "esp32-hal-log.h"

namespace {
  RTC_DATA_ATTR bool kPrev{false};
  RTC_DATA_ATTR std::atomic<int> kLock{0};
} // namespace

#include "soc/rtc_periph.h"
#include "rom/gpio.h"
#include "deep_sleep_utils.h"

void Power::lock() {
  if (kLock.fetch_add(1, std::memory_order_acq_rel) == 0) {
    high();
  }
}

void Power::unlock() {
  if (kLock.fetch_sub(1, std::memory_order_acq_rel) == 1) {
    low();
  }
}

void Power::set(bool high) {
  // The value changes based on the board! Old boards have the values fliped
  if (HW::kRevision >= 2)
    high = !high;

  // Caches previous values
  if (kPrev == high)
    return;

  // Not initialized in DeepSleep
  // const rtc_io_desc_t& desc = rtc_io_desc[rtc_io_num_map[HW::kVoltageSelectPin]];
  const rtc_io_desc_t desc = {RTC_IO_TOUCH_PAD4_REG, RTC_IO_TOUCH_PAD4_MUX_SEL_M, RTC_IO_TOUCH_PAD4_FUN_SEL_S, RTC_IO_TOUCH_PAD4_FUN_IE_M, RTC_IO_TOUCH_PAD4_RUE_M, RTC_IO_TOUCH_PAD4_RDE_M, RTC_IO_TOUCH_PAD4_SLP_SEL_M, RTC_IO_TOUCH_PAD4_SLP_IE_M, 0, RTC_IO_TOUCH_PAD4_HOLD_M, RTC_CNTL_TOUCH_PAD4_HOLD_FORCE_M, RTC_IO_TOUCH_PAD4_DRV_V, RTC_IO_TOUCH_PAD4_DRV_S, RTCIO_CHANNEL_14_GPIO_NUM};

  // Hold disable
  REG_CLR_BIT(RTC_CNTL_HOLD_FORCE_REG, desc.hold_force);
  REG_CLR_BIT(desc.reg, desc.hold);
  // Deep sleep hold disable
  CLEAR_PERI_REG_MASK(RTC_CNTL_DIG_ISO_REG, RTC_CNTL_DG_PAD_AUTOHOLD_EN_M);

  GPIO_MODE_OUTPUT(13);
  GPIO_OUTPUT_SET(HW::kVoltageSelectPin, high);

  // Hold enable
  REG_SET_BIT(RTC_CNTL_HOLD_FORCE_REG, desc.hold_force);
  REG_SET_BIT(desc.reg, desc.hold);

  // Deep sleep hold enable
  CLEAR_PERI_REG_MASK(RTC_CNTL_DIG_ISO_REG, RTC_CNTL_DG_PAD_FORCE_UNHOLD);
  SET_PERI_REG_MASK(RTC_CNTL_DIG_ISO_REG, RTC_CNTL_DG_PAD_AUTOHOLD_EN_M);

  kPrev = high;
}