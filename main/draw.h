#pragma once

#include "display.h"
#include "battery.h"
#include <TimeLib.h>
#include "ui.h"

struct DrawSettings {
    bool mInvert : 1 {false};
    bool mDarkBorder : 1 {false};
    uint8_t mRotation : 2 {2};

    // To optimize the drawing, if the previous one was a fatchface as well
    // This can save around 10ms (draw in CPU + SPI transfer) or 0.1uAh/draw
    struct {
        bool mValid{false}; // If the last draw was a watchface
        tmElements_t mTime{}; // Time & Date used
        uint16_t mBatery{0}; // Bat %
    } mLastDraw;    
};

/* This class handles the Draw of the display watchface
 * in an optimal way, caching elements to avoid redrawing
 */
class Draw {
    DrawSettings& mSettings;
    DisplayBW& mDisplay;
    const Battery& mBattery;
    const tmElements_t& mNow;

    void initialize();

    void time_hour(int16_t x, int16_t y);
    void time_mid(int16_t x, int16_t y);
    void time_min_01(int16_t x, int16_t y);
    void time_min_10(int16_t x, int16_t y);
    
    void date(int16_t x, int16_t y);
    void batteryIcon(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void battery(int16_t x, int16_t y);

    uint8_t mainColor() const;
    uint8_t backColor() const;

public:
    explicit Draw(DrawSettings& settings, DisplayBW& display, Battery& battery, const tmElements_t& now)
    : mSettings(settings)
    , mDisplay{display}
    , mBattery(battery)
    , mNow(now)
    {}

    //Main Entry points to draw to display
    void watchFace();
    void menu(const AnyItem& item, const uint8_t index);
};