
#include "core.h"
#include "power.h"
#include "touch.h"
#include "peripherals.h"
#include "hardware.h"

RTC_DATA_ATTR struct Settings {
    bool mValid {false};
    bool mTouchWatchDog {false};

    struct Menu {
        std::array<uint8_t, 4> mState{}; // Up to 4 levels deep (increase if needed)
        int8_t mDepth {-1};
    } mUi;    

    struct Clock {
        uint32_t mCalibration{16000000};
        bool mBeepHours : 1 {false};
        uint8_t mBeepHoursStart : 5 {0};
        uint8_t mBeepHoursEnd : 5 {24};
    } mClock;

    struct Display {
        bool mInvert : 1 {false};
        bool mDarkBorder : 1 {false};
        uint8_t mRotation : 2 {2};
    } mDisplay;

    TimeSettings mTime;
    TouchSettings mTouch;
    BatterySettings mBattery;
} kSettings;

// This is required since we initialize the Adafruit EPD with our EPD
Core::Core()
: mDisplay{Display{}}
, mTime{kSettings.mTime}
, mBattery{kSettings.mBattery}
, mTouch{kSettings.mTouch}
, mNow{mTime.getElements()}
, mUi{
MenuItem{"Main Menu", {
    MenuItem{"Clock", {
        // CustomItem{"Set Time", 
        //     [&](bool up) {

        //         mTime.adjustTime((up ? 1 : -1) * 60); 
        //     }
        //     [&]() {
        //         // Advance the selection to next field

        //     }
        // },
        MenuItem{"SetTime", {
            NumberItem{"Hour",
                [&]() -> int { return mNow.Hour; },
                [&](bool up){ mTime.adjustTime((up ? 1 : -1) * 3600); }
            },
            NumberItem{"Min",
                [&]() -> int { return mNow.Minute; },
                [&](bool up){ mTime.adjustTime((up ? 1 : -1) * 60); }
            },
            NumberItem{"Sec",
                [&]() -> int { return mNow.Second; },
                [&](bool up){ mTime.adjustTime(up ? 1 : -1); }
            },
        }},
        MenuItem{"SetDate", {
            NumberItem{"Year",
                [&]() -> int { return 1970 + mNow.Year; },
                [&](bool up){ /*mTime.adjustTime((up ? 1 : -1) * 3600);*/ }
            },
            NumberItem{"Month",
                [&]() -> int { return mNow.Month; },
                [&](bool up){ /*mTime.adjustTime((up ? 1 : -1) * 60);*/ }
            },
            NumberItem{"Day",
                [&]() -> int { return mNow.Day; },
                [&](bool up){ mTime.adjustTime((up ? 1 : -1) * 3600 * 24); }
            },
        }},
        MenuItem{"Calibration", {
            Item{"NOPE"},
            // {"Last", TextItem{}},
            // {"Drift", TextItem{}},
            /*ActionItem{[](){
                Peripherals::vibrator(std::vector<int>{75,75,75});}} },*/
        }},
        MenuItem{"Beep", {
            BoolItem{"Beep Hour",
                [](){return kSettings.mClock.mBeepHours; },
                [](bool val){ kSettings.mClock.mBeepHours = val; }
            },
            LoopItem{"St Hour",
                []() -> int { return kSettings.mClock.mBeepHoursStart; },
                [](){ kSettings.mClock.mBeepHoursStart = (kSettings.mClock.mBeepHoursStart + 1) % 24; }
            },
            LoopItem{"End Hour",
                []() -> int { return kSettings.mClock.mBeepHoursEnd; },
                [](){ kSettings.mClock.mBeepHoursEnd = (kSettings.mClock.mBeepHoursEnd + 1) % 24; }
            }
        }},
    }},
    MenuItem{"Display", {
        BoolItem{"Invert",
            [](){return kSettings.mDisplay.mInvert; },
            [](bool val){ kSettings.mDisplay.mInvert = val; }
        },
        BoolItem{"Border",
            [](){return kSettings.mDisplay.mDarkBorder; },
            [](bool val){ kSettings.mDisplay.mDarkBorder = val; }
        },
        LoopItem{"Rotation",
            []() -> int { return kSettings.mDisplay.mRotation; },
            [](){ kSettings.mDisplay.mRotation = (kSettings.mDisplay.mRotation + 1) % 4; }
        },
    }},
    Item{"Touch"},
    MenuItem{"Test", {
        ActionItem{"Vib 2x75ms", [](){
            Peripherals::vibrator(std::vector<int>{75,75,75});
        }},
        ActionItem{"Vib 1x75ms", [](){
            Peripherals::vibrator(std::vector<int>{75});
        }},
        ActionItem{"Vib 200ms", [](){
            Peripherals::vibrator(std::vector<int>{200});
        }},
        ActionItem{"Scale", [](){
            Peripherals::speaker(std::vector<std::pair<int,int>>{
                {200,1000},{400,1000},{800,1000},{1600,1000},{3200,1000},
                {6400,1000},{10000,1000},{12000,1000}});
            delay(8000);
        }},
        ActionItem{"Beep", [](){
            Peripherals::speaker(
                std::vector<std::pair<int,int>>{
                {3200,500},{0,500},{3200,500}
            });    
        }},
        ActionItem{"Tetris", [](){
            Peripherals::tetris(); 
        }},
    }},
}}}
{}

void Core::boot() {


    //ESP_LOGE("deepSleep", "boot %ld", millis());
    mDisplay.epd2.initDisplay(); // TODO: Move it to constructor
    
    if (kSettings.mValid) {
        // Recover Settings from Disk
        kSettings.mValid = true;
    }

    // Beep conditions
    if (kSettings.mClock.mBeepHours) {
        if (mNow.Minute == 0 
            && mNow.Hour > kSettings.mClock.mBeepHoursStart
            && mNow.Hour < kSettings.mClock.mBeepHoursEnd)
            Peripherals::speaker(
                std::vector<std::pair<int,int>>{
                {3200,100},{0,500},{3200,500}
            });
    }

    //Wake up reason affects how to proceed
    auto wakeup_reason = esp_sleep_get_wakeup_cause();
    switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_TOUCHPAD: { // Touch!
        auto touch_pad = esp_sleep_get_touchpad_wakeup_status();
        if (touch_pad != TOUCH_PAD_MAX) {
            ESP_LOGE("", "pad %d", (int)touch_pad);
            handleTouch(touch_pad);
        } else {
            ESP_LOGE("Touch", "TouchPad error");
        } 
    } break;
    case ESP_SLEEP_WAKEUP_TIMER: // Internal Timer 
    case ESP_SLEEP_WAKEUP_EXT0: // RTC Alarm ?
        // Check alarms, vibration, beeps, etc.

        // Check watchdog -> reset to watchFace
        if (kSettings.mTouchWatchDog) {
            kSettings.mUi.mDepth = -1; // Reset to watchFace
        }
        kSettings.mTouchWatchDog = true;

        break;
    default: // Lets assume first time boot
        firstTimeBoot();
        break;
    }

    prepareDisplay();

    // Show watch face or menu
    if (kSettings.mUi.mDepth < 0) {
        showWatchFace();
    } else {
        showUi();
    }

    deepSleep();
}

void Core::firstTimeBoot() {
    // For some reason, seems to be enabled on first boot
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    // Select default voltage 2.6V
    Power::low();
    // HACK: Set a fixed time 
    struct timeval tv{.tv_sec=1716064825, .tv_usec=0};
    struct timezone tz{.tz_minuteswest=60, .tz_dsttime=1};
    mTime.setTime(tv);
}

const AnyItem& Core::findUi() const {
    // Find current UI element in view by recursively finding deeper elements
    const AnyItem* item = &mUi;
    for (auto i=0; i < kSettings.mUi.mDepth; i++) {
        auto index = kSettings.mUi.mState[i]; 
        std::visit([&](auto&& e){
            using T= std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, MenuItem>) {
                // Menu items return its subelements
                item = &e.items[index];
            }
        }, *item);
    }
    return *item;
}

void Core::handleTouch(const touch_pad_t touch_pad) {
    // Clear WatchDog since we received a valid touch
    kSettings.mTouchWatchDog = false;

    // Convert to what button was it
    // TODO: Unmap the mMap back
    Touch::Btn btn = static_cast<Touch::Btn>(HW::Touch::Num2Btn[touch_pad]);

    auto& ui = kSettings.mUi;
    ESP_LOGE("ui", "depth%d st%d", ui.mDepth, ui.mState[ui.mDepth]);

    // Button press on the watchface
    if (ui.mDepth < 0) {
        if (btn == Touch::Light)
            ; //Todo
        else if (btn == Touch::Menu) {
            ui.mDepth = 0;
            // ui.mState[ui.mDepth] = 0; // Do not set it to 0, remember value
        }
        return;
    }

    // Button press on the UI
    auto item = findUi();

    if (btn == Touch::Back) {
        // A back button always goes back in the depth, all elements
        ui.mDepth--;
    } else if (btn == Touch::Menu) {
        std::visit([&](auto&& e){
            using T= std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, MenuItem>) {
                // When we press in a menu, we need to check the type of the current subelement
                std::visit([&](auto&& sub){
                    using U= std::decay_t<decltype(sub)>;
                    if constexpr (std::is_same_v<U, MenuItem>) {
                        // Another menu, enter it, then sanitize
                        ui.mDepth++;
                        ui.mState[ui.mDepth] = std::clamp(ui.mState[ui.mDepth], (uint8_t)0, (uint8_t)(sub.items.size() - 1));
                    } else if constexpr (std::is_same_v<U, NumberItem>) {
                        ui.mDepth++;
                    } else if constexpr (std::is_same_v<U, ActionItem>) {
                        sub.action();
                    } else if constexpr (std::is_same_v<U, BoolItem>) {
                        sub.toggle();
                    } else if constexpr (std::is_same_v<U, LoopItem>) {
                        sub.tick();
                    }
                }, e.items[ui.mState[ui.mDepth]]);
            } else {
                ;
            }
        }, item);
    } else if (btn == Touch::Up) {
        std::visit([&](auto&& e){
            using T= std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, MenuItem>) {
                auto size = e.items.size();
                ui.mState[ui.mDepth] = (ui.mState[ui.mDepth] + size - 1) % size;
            } else if constexpr (std::is_same_v<T, NumberItem>) {
                e.change(true);
            } else {
                ;
            }
        }, item);
    } else if (btn == Touch::Down) {
        std::visit([&](auto&& e){
            using T= std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, MenuItem>) {
                auto size = e.items.size();
                ui.mState[ui.mDepth] = (ui.mState[ui.mDepth] + size + 1) % size;
            } else if constexpr (std::is_same_v<T, NumberItem>) {
                e.change(false);
            } else {
                ;
            }
        }, item);
    } 
    ESP_LOGE("ui", "depth%d st%d", ui.mDepth, ui.mState[ui.mDepth]);
}

void Core::showUi() {
    mDisplay.fillScreen(GxEPD_WHITE);

    // Text size for all the UI
    mDisplay.setTextSize(3);

    auto item = findUi();
    std::visit([&](auto&& e){
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, MenuItem>) {
            auto index = kSettings.mUi.mState[kSettings.mUi.mDepth];
            // Print the menu title on top, centered
            mDisplay.setTextColor(GxEPD_BLACK, GxEPD_WHITE);
            mDisplay.print(" ");
            mDisplay.println(e.name);
            //mDisplay.setCursor(mDisplay.getCursorX(), mDisplay.getCursorY() + 5);
            for(auto i = 0;i < e.items.size(); i++) {
                if (i == index) {
                    mDisplay.setTextColor(GxEPD_WHITE, GxEPD_BLACK);
                } else {
                    mDisplay.setTextColor(GxEPD_BLACK, GxEPD_WHITE);
                }
                // Depending on the menuitem type we might print differently
                std::visit([&](auto&& sub){
                    using U = std::decay_t<decltype(sub)>;
                    if constexpr (std::is_same_v<U, BoolItem>) {
                        // BoolItem show value + name
                        mDisplay.print(sub.get() ? "O " : "X ");
                        mDisplay.println(sub.name);
                    } else if constexpr (std::is_same_v<U, LoopItem>) {
                        // BoolItem show value + name
                        mDisplay.print(sub.get());
                        mDisplay.print(' ');
                        mDisplay.println(sub.name);
                    } else if constexpr (std::is_same_v<U, NumberItem>) {
                        // BoolItem show value + name
                        mDisplay.print(sub.get());
                        mDisplay.print(' ');
                        mDisplay.println(sub.name);
                    } else {
                        // Default just print the name
                        mDisplay.println(sub.name);
                    }
                }, e.items[i]);
            }
        } else if constexpr (std::is_same_v<T, NumberItem>) {
            // TODO: An Action Item renders its upper level menu, but with a selection mark
            mDisplay.setTextColor(GxEPD_BLACK, GxEPD_WHITE);
            mDisplay.println(e.get());
        }
    }, item);

    mDisplay.display(true);
}

#include <Fonts/FreeMonoBold9pt7b.h>
#include "DSEG7_Classic_Bold_53.h"
#include "Seven_Segment10pt7b.h"
#include "DSEG7_Classic_Regular_15.h"
#include "DSEG7_Classic_Bold_25.h"
#include "DSEG7_Classic_Regular_39.h"
#include "icons.h"

void Core::drawTime(int16_t x, int16_t y){
    mDisplay.setFont(&DSEG7_Classic_Bold_53);
    mDisplay.setCursor(x, y);
    int displayHour;
      displayHour = mNow.Hour;
    if(displayHour < 10){
        mDisplay.print("0");
    }
    mDisplay.print(displayHour);
    mDisplay.print(":");
    if(mNow.Minute < 10){
        mDisplay.print("0");
    }
    mDisplay.println(mNow.Minute);
}
void Core::drawDate(int16_t x, int16_t y){
    mDisplay.setFont(&Seven_Segment10pt7b);

    mDisplay.setCursor(x, y);

    // const char * weekDay = "SSMTWTF";
    // for (int i = 1; i<8; i++) {
    //     if (i == mNow.Wday) {
    //         mDisplay.setTextColor(GxEPD_WHITE, GxEPD_BLACK);
    //         mDisplay.getTextBounds(String(weekDay[i%7]), mDisplay.getCursorX(), mDisplay.getCursorY(), &x1, &y1, &w, &h);
    //         mDisplay.fillRect(mDisplay.getCursorX(), mDisplay.getCursorY()-h, w+2, h+2, GxEPD_BLACK);
    //     } else {
    //         mDisplay.setTextColor(GxEPD_BLACK, GxEPD_WHITE);
    //     }
    //     mDisplay.print(weekDay[i%7]);
    // }

    String dayOfWeek = dayShortStr(mNow.Wday);
    mDisplay.setCursor(x, y);
    mDisplay.println(dayOfWeek);

    String month = monthShortStr(mNow.Month);
    mDisplay.setCursor(x + 70, y);
    mDisplay.println(month);

    mDisplay.setFont(&DSEG7_Classic_Regular_15);
    mDisplay.setCursor(x + 40, y+1);
    if(mNow.Day < 10){
        mDisplay.print("0");
    }
    mDisplay.println(mNow.Day);
    mDisplay.setCursor(x + 110, y+1);
    mDisplay.println(tmYearToCalendar(mNow.Year));// offset from 1970, since year is stored in uint8_t
}

void Core::showWatchFace() {
    // FROM V0
    bool DARKMODE = false;
    mDisplay.fillScreen(DARKMODE ? GxEPD_BLACK : GxEPD_WHITE);
    mDisplay.setTextColor(DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    drawTime(4, 73);
    drawDate(17, 97);
    // drawBattery(100, 73+150);
    // mDisplay.print(mBattery.mCurVoltage);
    mDisplay.setCursor(68, 120);
    mDisplay.setFont(NULL);
    mDisplay.setTextSize(2);
    mDisplay.printf("%.1f%%", mBattery.mCurPercent * 0.1f);

    // mDisplay.fillScreen(GxEPD_WHITE);
    // mDisplay.setTextColor(GxEPD_BLACK);
    // mDisplay.setTextSize(4);
    // mDisplay.print("ASD");
    mDisplay.display(!mDisplay.epd2.displayFullInit);
    delay(2);
}
void Core::drawBatteryIcon(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    mDisplay.drawRect(x + 2, y + 0, w - 4, h - 0, color);
    mDisplay.drawRect(x + 1, y + 1, w - 2, h - 2, color);
    mDisplay.drawRect(x + 0, y + 2, w - 0, h - 4, color);
    // Pointy end
    // mDisplay.drawRect(x + w, y + 4, 2, h - 8, color);
}
void Core::drawBattery(int16_t x, int16_t y) {
    bool DARKMODE = false;

    //mDisplay.drawBitmap(154, 73, battery, 37, 21, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    drawBatteryIcon(140, 73, 55, 23, GxEPD_BLACK);

    mDisplay.setTextSize(2);
    mDisplay.setFont(NULL);
    // mDisplay.setFont(&FreeMonoBold9pt7b);
    // mDisplay.setFont(&Seven_Segment10pt7b);

    mDisplay.setCursor(142, 77);
    float perc = mBattery.mCurPercent;
    mDisplay.printf("%.1f", perc+90);
    //display.fillRect(159, 78, 27, BATTERY_SEGMENT_HEIGHT, DARKMODE ? GxEPD_BLACK : GxEPD_WHITE);//clear battery segments
    // if(VBAT > 4.1){
    //     batteryLevel = 3;
    // }
    // else if(VBAT > 3.95 && VBAT <= 4.1){
    //     batteryLevel = 2;
    // }
    // else if(VBAT > 3.80 && VBAT <= 3.95){
    //     batteryLevel = 1;
    // }
    // else if(VBAT <= 3.80){
    //     batteryLevel = 0;
    // }

    // for(int8_t batterySegments = 0; batterySegments < batteryLevel; batterySegments++){
    //     display.fillRect(159 + (batterySegments * BATTERY_SEGMENT_SPACING), 78, BATTERY_SEGMENT_WIDTH, BATTERY_SEGMENT_HEIGHT, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    // }
}

/*void Core::NTPSync() {
    // Select default voltage 2.9V for WiFi
    Power::high();
    //sleep(3);
    // We need Arduino for this (WiFi + NTP)
    initArduino();
    showSyncNTP();
    // Select default voltage 2.6V
    setVoltage(false);
}*/

void Core::deepSleep() {
    mDisplay.hibernate();

    // Set GPIOs 0-39 to input to avoid power leaking out
    const uint64_t ignore = 0b11110001000000110000100111000010; // Ignore some GPIOs due to resets
    for (int i = 0; i < GPIO_NUM_MAX; i++) {
        if ((ignore >> i) & 0b1)
            continue;
        // ESP_LOGE("", "%d input", i);
        pinMode(i, INPUT);
    }
    Power::low(); // Needed? Can it be remembered? Cached TODO

    mTouch.setUp(kSettings.mUi.mDepth < 0);

    // ESP_LOGE("deepSleep", "%ld", millis());

    //esp_deep_sleep_disable_rom_logging();
    //esp_sleep_enable_timer_wakeup(1000000 - mTime.getTimeval().tv_usec);
    // TODO SLEEP PLANING
    if (mBattery.mCurPercent < 100 || mNow.Hour < 7) {
        esp_sleep_enable_timer_wakeup((5 * 60 - mNow.Second) * 1000000 - mTime.getTimeval().tv_usec);
    } else if (mBattery.mCurPercent < 200) {
        esp_sleep_enable_timer_wakeup((4 * 60 - mNow.Second) * 1000000 - mTime.getTimeval().tv_usec);
    } else if (mBattery.mCurPercent < 500) {
        esp_sleep_enable_timer_wakeup((2 * 60 - mNow.Second) * 1000000 - mTime.getTimeval().tv_usec);
    } else {
        esp_sleep_enable_timer_wakeup((60 - mNow.Second) * 1000000 - mTime.getTimeval().tv_usec);
    }

    //esp_sleep_enable_timer_wakeup(10*1000000);
    esp_deep_sleep_start();
    ESP_LOGE("deepSleep", "never reach!");
}

// This function is called when the display will be used soon
// and we need to setup the defaults from RTC/Settings
void Core::prepareDisplay() {
  mDisplay.setRotation(kSettings.mDisplay.mRotation);
  mDisplay.epd2.setDarkBorder(kSettings.mDisplay.mDarkBorder ^ kSettings.mDisplay.mInvert);
  mDisplay.epd2.inverted = kSettings.mDisplay.mInvert;
  mDisplay.epd2.asyncPowerOn();
}