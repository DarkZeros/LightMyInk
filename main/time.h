#pragma once

#include <time.h>
#include <TimeLib.h>

struct TimeSettings {
    uint8_t mZone{0};
    time_t mLastSync{0};
    int32_t mDrift{0};
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
    void adjustTime(int32_t seconds);
    void setTime(const timeval&);

    const timeval& getTimeval() const { return mTv; }
    const tmElements_t& getElements() const { return mElements; }
};