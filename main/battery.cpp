
#include "battery.h"

#include "esp_adc/adc_oneshot.h"

#include "Arduino.h"

Battery::Battery(BatterySettings& settings)
: mSettings{settings}
, mCurVoltage{readVoltageScaledAveraged()}
, mCurPercent{percent()}
{
}

uint16_t Battery::readVoltageScaledAveraged() const {
    // Perform N reads and average them
    uint32_t total{};
    for(auto i=0; i<mSettings.mSamples; i++) {
        total += readVoltageScaled();
    }
    total /= mSettings.mSamples;
    // The merge that data with the running average
    auto& av = mSettings.mVoltage;
    if (av == 0xFFFF) {
        // Still no average, use this value as starting point
        av = total;
    } else {
        av += (total - mSettings.mVoltage) / mSettings.mAverages;
    }
    return av;
}

uint16_t Battery::readVoltageScaled() const {
    // adc_oneshot_unit_handle_t adc_handle;
    // adc_oneshot_unit_init_cfg_t init_config = {
    //     .unit_id = ADC_UNIT_1,
    //     .ulp_mode = ADC_ULP_MODE_DISABLE,
    // };
    // ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));
    // adc_oneshot_chan_cfg_t config = {
    //     .atten = ADC_ATTEN_DB_0,
    //     .bitwidth = ADC_BITWIDTH_DEFAULT,
    // };
    // adc_unit_t unit;
    // adc_channel_t channel;
    // adc_oneshot_io_to_channel(HW::kAdcPin, &unit, &channel);
    // ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, channel, &config));
    // int reading;
    // adc_oneshot_read(adc_handle, channel, &reading);

    // //uint32_t reading = adc1_get_raw(HW::kAdcPin);
    // //uint32_t voltage = adc_cali_raw_to_voltage(reading, adc_chars);
    // return reading;

    // Battery voltage goes through a 1/2 divider, we take care later of it
    return analogReadMilliVolts(HW::kAdcPin) * mSettings.mScale / 64;
}

uint16_t Battery::percent() const {
    auto it = std::lower_bound(kLipoVolt2Perc.begin(), kLipoVolt2Perc.end(), 
        std::pair<uint8_t, uint16_t>{0, mCurVoltage},
        [](auto& a, auto& b) { return a.second < b.second; }
    );
    // ESP_LOGE("", "%d %d,  %d %d", (it-1)->first, (it-1)->second, it->first, it->second);
    return (
        (mCurVoltage - (it-1)->second) * it->first + (it->second - mCurVoltage) * (it-1)->first) * 10
         / (it->second - (it-1)->second);
}