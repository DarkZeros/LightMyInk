
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

    struct Menu {
        std::array<uint8_t, 4> mState{}; // Up to 4 levels deep (increase if needed)
        int8_t mDepth {-1};
    } mUi;

    struct Clock {
        uint32_t mCalibration{16000000};
        bool mHourlyBeep : 1 {false};
        bool mHourlyVib : 1 {false};
        uint8_t mHourlyStart : 5 {0};
        uint8_t mHourlyEnd : 5 {24};
    } mClock;

    TimeSettings mTime;
    TouchSettings mTouch;
    BatterySettings mBattery;
    DisplaySettings mDisplay;
    WatchfaceSettings mWatchface;
};
extern struct Settings kSettings;