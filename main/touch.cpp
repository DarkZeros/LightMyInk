#include "touch.h"
#include "hardware.h"

#include "driver/touch_sensor.h"
#include "esp_sleep.h"

// This takes around 0.4ms
// TODO: Is this needed all the time? Can't it be set up once?
// touch_pad_fsm_start()
void Touch::setUp(bool onlyMenuLight) {
  touch_pad_init();
  touch_pad_set_voltage(TOUCH_HVOLT_2V5, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_0V); 
  //touch_pad_set_cnt_mode(); 
  touch_pad_set_measurement_clock_cycles(mSettings.mMeasCycles);
  touch_pad_set_measurement_interval(mSettings.mMeasInterval);
  //touch_pad_set_trigger_mode(TOUCH_TRIGGER_BELOW);
  //touch_pad_intr_enable();  // returns ESP_OK
  esp_sleep_enable_touchpad_wakeup();
  //touch_pad_denoise_disable();
  touch_pad_config((touch_pad_t)HW::Touch::Num[mSettings.mMap[Touch::Menu]], mSettings.mThreshold);
  touch_pad_config((touch_pad_t)HW::Touch::Num[mSettings.mMap[Touch::Light]], mSettings.mThreshold);
  if (!onlyMenuLight) {
    touch_pad_config((touch_pad_t)HW::Touch::Num[mSettings.mMap[Touch::Down]], mSettings.mThreshold);
    touch_pad_config((touch_pad_t)HW::Touch::Num[mSettings.mMap[Touch::Up]], mSettings.mThreshold);
  }
  // Touch Sensor Timer initiated
  touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);   // returns ESP_OK
  
  uint16_t mask =
    (1 << HW::Touch::Num[mSettings.mMap[Touch::Menu]])
    |(1 << HW::Touch::Num[mSettings.mMap[Touch::Light]])
    |(!onlyMenuLight << HW::Touch::Num[mSettings.mMap[Touch::Down]])
    |(!onlyMenuLight << HW::Touch::Num[mSettings.mMap[Touch::Up]]);

  touch_pad_set_group_mask(mask, mask, mask);
}