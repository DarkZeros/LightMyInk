#pragma once

#include "display.h"
#include "watchface.h"
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
    void deepSleep(); // Set up device for deep sleep

    const AnyItem& findUi() const;
    void handleTouch(const touch_pad_t touch_pad);
    void showMenu(const AnyItem& item, const uint8_t index);

    Display mDisplay;
    Time mTime;
    Battery mBattery;
    Touch mTouch;

    const tmElements_t& mNow;
    const AnyItem mUi;
};