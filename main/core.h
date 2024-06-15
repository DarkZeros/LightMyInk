#pragma once

#include "display.h"
#include "battery.h"
#include "touch.h"
#include "ui.h"
#include "time.h"

/* This is the primary class of the project.
 * It has the entry point from deepsleep as well as all
 * the code that handles the setup/menus/misc.
 */
class Core {
public:
    void boot(); // Called when ESP32 starts up

    Core();

private:
    void firstTimeBoot();
    void prepareDisplay();
    void deepSleep(); // Set up device for deep sleep

    const AnyItem& findUi() const;
    void handleTouch(const touch_pad_t touch_pad);
    void showUi();
    void showWatchFace();

    void drawTime(int16_t x, int16_t y);
    void drawDate(int16_t x, int16_t y);
    void drawBatteryIcon(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void drawBattery(int16_t x, int16_t y);

    uint8_t mainColor() const;
    uint8_t backColor() const;

    DisplayBW mDisplay;
    Time mTime;
    Battery mBattery;
    Touch mTouch;
    
    const tmElements_t& mNow; 
    AnyItem mUi;
};