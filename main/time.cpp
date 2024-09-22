#include "time.h"
#include "sys/time.h"
#include "esp32-hal-log.h"
#include "esp_private/esp_clk.h"

#include <algorithm>

void Time::readTime() {
    gettimeofday(&mTv, NULL);
    // TODO: Apply timezone correction manually, since IDF does not handle it
    // timezone t;
    breakTime(mTv.tv_sec, mElements);
}

void Time::adjustTime(const timeval& time) {
    gettimeofday(&mTv, NULL); // redundant? Maybe 2ms drift?
    mTv.tv_sec += time.tv_sec;
    mTv.tv_usec += time.tv_usec;
    setTime(mTv);
}

void Time::adjustTime(const int32_t& seconds) {
    struct timeval time{seconds, 0};
    adjustTime(time);
}

void Time::setTime(const tmElements_t& elements) {
    setTime(makeTime(elements));
}

void Time::setTime(const time_t& seconds) {
    struct timeval time{seconds, 0};
    setTime(time);
}

void Time::setTime(const timeval& tm) {
    gettimeofday(&mTv, NULL); // redundant? Maybe 2ms drift?
    // Store difference in drift
    if (mSettings.mSync) {
        mSettings.mSync->mDrift.tv_sec += tm.tv_sec - mTv.tv_sec;
    }
    // Set it and update times
    settimeofday(&tm, NULL);
    readTime();
}

void Time::calSet() {
    esp_clk_slowclk_cal_set(mSettings.mCalibration);
}
void Time::calUpdate() {
    if (!mSettings.mSync) {
        mSettings.mSync.emplace(mTv, timeval{});
        return;
    }
    auto& sync = *mSettings.mSync;
    auto& time = sync.mTime;
    double elapsed = (mTv.tv_sec - time.tv_sec) + (mTv.tv_usec - time.tv_usec) * 0.000'001;
    double drift = sync.mDrift.tv_sec + sync.mDrift.tv_usec * 0.000'001;
    uint32_t cal = 1.0 * mSettings.mCalibration * elapsed / (elapsed - drift);

    // ESP_LOGE("cal", "drift %f elapsed %f, cal %lu", drift, elapsed, cal);
    // Update drift, time and calibration
    drift -= (1.f * cal - mSettings.mCalibration) / mSettings.mCalibration * (elapsed - drift);
    sync.mDrift.tv_sec = drift;
    drift -= sync.mDrift.tv_sec;
    sync.mDrift.tv_usec = drift * 1'000'000;
    // time = mTv; // Do not change time, carry over from the start
    mSettings.mCalibration = std::clamp(cal, kDefaultCalibration / 10, kDefaultCalibration * 10);

    calSet();
}
void Time::calReset() {
    mSettings.mSync.reset();
    mSettings.mCalibration = kDefaultCalibration;
    calSet();
}