
#include "core.h"
#include "power.h"
#include "touch.h"
#include "peripherals.h"
#include "hardware.h"
#include "settings.h"

RTC_DATA_ATTR Settings kSettings;

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
        MenuItem{"Hour Notify", {
            BoolItem{"Beep",
                [](){return kSettings.mClock.mHourlyBeep; },
                [](bool val){ kSettings.mClock.mHourlyBeep = val; }
            },
            BoolItem{"Vibrate",
                [](){return kSettings.mClock.mHourlyVib; },
                [](bool val){ kSettings.mClock.mHourlyVib = val; }
            },
            LoopItem{"St Hour",
                []() -> int { return kSettings.mClock.mHourlyStart; },
                [](){ kSettings.mClock.mHourlyStart = (kSettings.mClock.mHourlyStart + 1) % 24; }
            },
            LoopItem{"End Hour",
                []() -> int { return kSettings.mClock.mHourlyEnd; },
                [](){ kSettings.mClock.mHourlyEnd = (kSettings.mClock.mHourlyEnd + 1) % 24; }
            }
        }},
    }},
    MenuItem{"Display", {
        BoolItem{"Invert",
            [](){return kSettings.mDraw.mInvert; },
            [](bool val){ kSettings.mDraw.mInvert = val; }
        },
        BoolItem{"Border",
            [](){return kSettings.mDraw.mDarkBorder; },
            [](bool val){ kSettings.mDraw.mDarkBorder = val; }
        },
        LoopItem{"Rotation",
            []() -> int { return kSettings.mDraw.mRotation; },
            [](){ kSettings.mDraw.mRotation = (kSettings.mDraw.mRotation + 1) % 4; }
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
                {3200,100},{0,500},{3200,100}
            });    
        }},
        ActionItem{"Tetris", [](){
            Peripherals::tetris(); 
        }},
    }},
}}
}
, mDraw{kSettings.mDraw, mDisplay, mBattery, mNow}
{
    // ESP_LOGE("", "cend %lu", micros());
}

void Core::boot() {

    //ESP_LOGE("", "boot %lu", micros());
    
    if (kSettings.mValid) {
        // Recover Settings from Disk // TODO
        kSettings.mValid = true;
    }

    //Wake up reason affects how to proceed
    auto wakeup_reason = esp_sleep_get_wakeup_cause();
    switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_TOUCHPAD: { // Touch!
        auto touch_pad = esp_sleep_get_touchpad_wakeup_status();
        if (touch_pad != TOUCH_PAD_MAX) {
            // ESP_LOGE("pad", "%d", (int)touch_pad);
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

    // Beep conditions
    if (kSettings.mClock.mHourlyBeep) {
        if (mNow.Minute == 0 
            && wakeup_reason == ESP_SLEEP_WAKEUP_TIMER
            && mNow.Hour > kSettings.mClock.mHourlyStart
            && mNow.Hour < kSettings.mClock.mHourlyEnd)
            Peripherals::speaker(
                std::vector<std::pair<int,int>>{
                {3200,100},{0,500},{3200,500}
            });
    }

    // Show watch face or menu ?
    if (kSettings.mUi.mDepth < 0) {
        mDraw.watchFace();
    } else {
        mDraw.menu(findUi(), kSettings.mUi.mState[kSettings.mUi.mDepth]);
    }

    deepSleep();
}

void Core::firstTimeBoot() {
    // For some reason, seems to be enabled on first boot
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    // Select default voltage 2.6V
    Power::low();
    // HACK: Set a fixed time 
    struct timeval tv{.tv_sec=1718665000, .tv_usec=0};
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
    // ESP_LOGE("ui", "depth%d st%d", ui.mDepth, ui.mState[ui.mDepth]);

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
    // ESP_LOGE("ui", "depth%d st%d", ui.mDepth, ui.mState[ui.mDepth]);
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

void extern RTC_IRAM_ATTR wake_stub_example(void);

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

    ESP_LOGE("deepSleep", "%ld", micros());

    // Wake up stub ?
    // esp_set_deep_sleep_wake_stub(&wake_stub_example);

    esp_sleep_enable_timer_wakeup(1000000 - mTime.getTimeval().tv_usec);
    esp_deep_sleep_start();
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