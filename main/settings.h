
#pragma once

#include "battery.h"
#include "time.h"
#include "touch.h"
#include "watchface.h"
#include "display.h"

struct Settings {
    bool mValid : 1 {false};
    bool mTouchWatchDog : 1 {false};
    bool mLeakPinsSet : 1 {false};

    struct Hourly {
        bool mBeep : 1 {false};
        bool mVib : 1 {false};
        uint8_t mStart : 5 {0};
        uint8_t mEnd : 5 {24};
    } mHourly;

    UiSettings mUi;
    TimeSettings mTime;
    TouchSettings mTouch;
    BatterySettings mBattery;
    DisplaySettings mDisplay;
    WatchfaceSettings mWatchface;

    struct PowerSave {
        bool mNight {false};
        bool mAuto {true};
    } mPowerSave;
};
extern struct Settings kSettings;