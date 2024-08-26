#include "time.h"
#include "sys/time.h"
#include "esp32-hal-log.h"
#include "esp_private/esp_clk.h"

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
    mSettings.mDrift += tm.tv_sec - mTv.tv_sec;
    // Set it and update times
    settimeofday(&tm, NULL);
    readTime();
}

void Time::cal() {
    esp_clk_slowclk_cal_set(mSettings.mCalibration);
}
void Time::calSync() {
    // newCal = cal * -drift / elapsed;
    if (mSettings.mLastSync == 0) {
        calReset();
        return;
    }
    const auto elapsed = mTv.tv_sec - mSettings.mLastSync;
    ESP_LOGE("old", "elapsed %lld cal %ld drift%ld", elapsed, mSettings.mCalibration, mSettings.mDrift);

    double newCal = 1.0 * mSettings.mCalibration * (elapsed) / (elapsed - mSettings.mDrift);
    mSettings.mCalibration = newCal;
    // Calculate error and add it to drift
    double error = newCal - mSettings.mCalibration;
    // Keep in the drift what could not be accounted for
    mSettings.mDrift = static_cast<uint32_t>(error * elapsed / mSettings.mCalibration);
    cal();

    ESP_LOGE("cal", "elapsed %lld cal %ld newCal %f error%f newDrift%ld", elapsed, mSettings.mCalibration, newCal, error, mSettings.mDrift);
}
void Time::calReset() {
    mSettings.mLastSync = mTv.tv_sec;
    mSettings.mDrift = 0;
    mSettings.mCalibration = mSettings.kDefaultCalibration;
    cal();
}