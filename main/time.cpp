#include "time.h"
#include "sys/time.h"

void Time::readTime() {
    gettimeofday(&mTv, NULL);
    // Apply timezone correction manually, since IDF does not handle it
    // timezone t;
    // 
    breakTime(mTv.tv_sec, mElements);
}

void Time::adjustTime(int32_t seconds) {
    gettimeofday(&mTv, NULL); // redundant? Maybe 2ms drift?
    mTv.tv_sec += seconds;
    settimeofday(&mTv, NULL);
    breakTime(mTv.tv_sec, mElements);
}

void Time::setTime(const timeval& tm) {
    settimeofday(&tm, NULL);
    mTv = tm;
    breakTime(mTv.tv_sec, mElements);
}