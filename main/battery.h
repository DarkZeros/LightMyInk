#pragma once

#include "hardware.h"

#include <array>
#include <time.h>
#include <utility>

struct BatterySettings {
    uint8_t mScale{128}; // Adjust the scaler, Divided by 64, default 2x
    uint8_t mSamples{1};

    // Runing average battery indicator
    uint16_t mVoltage{0xFFFF};
    uint8_t mAverages{16}; // How much to perform the running average

    // Historic data ? TODO
    time_t mFirstMeasure{0}; // When we started the bat stats
    time_t mLastMeasure{0}; // When we did last measurement
    // The diff of both give us where we store/aggregate
    // ..
    /*uint16_t mDays[7]{}; // Last 7 days measures
    uint16_t mHours[24]{}; // Last 24h measures
    uint16_t mMinutes[60]; // Last 60m measures
    uint8_t mDayTick : 3;
    uint8_t mHourTick : 5;
    uint8_t mMinuteTick : 6;*/
};

constexpr std::array<std::pair<uint8_t, uint16_t>, 16> kLipoVolt2Perc = {{
    {0, 0},
    {0, 2000},
    {1, 3000},
    {2, 3200},
    {5, 3400},
    {10, 3600},
    {20, 3650},
    {30, 3700},
    {40, 3720},
    {50, 3750},
    {60, 3800},
    {70, 3860},
    {80, 3950},
    {90, 4050},
    {100, 4200},
    {100, 6000}
}};

/* This class handles the reading and estimating battery capacity
** It can also guess the power usage and the reminaing battery time
*/
class Battery {
    BatterySettings& mSettings;
    
    uint16_t readVoltageScaled() const;
    uint16_t readVoltageScaledAveraged() const;
    uint16_t percent() const;

public:
    explicit Battery(BatterySettings& settings);

    const uint16_t mCurVoltage; // In mV
    const uint16_t mCurPercent; // In fixed point 0.0% units
};