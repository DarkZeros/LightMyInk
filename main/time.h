#pragma once

#include <time.h>
#include <TimeLib.h>

struct TimeSettings {
    // uSeconds in Q13.19 fixed-point
    // 1/32768 * 2^19 = 16'000'000
    constexpr static uint32_t kDefaultCalibration{16'000'000};
    uint32_t mCalibration{kDefaultCalibration};
    // uint8_t mZone{0};
    time_t mLastSync{};
    int32_t mDrift{};
};

/* This class handles the time
 * It has epoch / tmElement and can handle timezones
*/
class Time {
    TimeSettings& mSettings;

    timeval mTv;
    //struct timezone mTimezone;
    tmElements_t mElements;

public:
    explicit Time(TimeSettings& settings) : mSettings{settings} {readTime();};

    void readTime();
    void setTime(const timeval&);
    // Wrappers
    void adjustTime(const timeval& time);
    void adjustTime(const int32_t& seconds);
    void setTime(const tmElements_t& elements);
    void setTime(const time_t&);

    // Calibration
    void cal();
    void calSync();
    void calReset();

    const timeval& getTimeval() const { return mTv; }
    const tmElements_t& getElements() const { return mElements; }
};