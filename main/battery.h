#pragma once

#include "hardware.h"

// Settings stored in RTC/NVS
struct BatteryConfig {
    uint8_t mScale{128}; // Divided by 64, default 2x
    // ..
};

/* This class handles the reading and estimating battery capacity
** It can also guess the power usage and the reminaing battery time
*/
class Battery {
    uint8_t readVoltage() const {
        // Battery voltage goes through a 1/2 divider.
        return analogReadMilliVolts(HW::kAdcPin) * 0.002f;
    }
public:
    uint8_t voltage() const;


};