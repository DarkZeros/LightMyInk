#include "core.h"
#include "deep_sleep.h"
#include "power.h"
#include "light.h"
#include "touch.h"
#include "peripherals.h"
#include "hardware.h"
#include "settings.h"

#include "watchface_default.h"

RTC_DATA_ATTR Settings kSettings;

// This is required since we initialize the Adafruit EPD with our EPD
Core::Core()
: mDisplay{}
, mTime{kSettings.mTime}
, mBattery{kSettings.mBattery}
, mTouch{kSettings.mTouch}
, mNow{mTime.getElements()}
, mUi{
UI::Menu{"Main Menu", {
    UI::Menu{"Clock", {
        UI::DateTime{"Set DateTime", mTime},
        UI::Menu{"Calibration", {
            UI::Action{"Sync", [&](){
                mTime.calUpdate();
            }},
            UI::Action{"Reset", [&](){
                mTime.calReset();
            }},
            UI::Text{[&]() -> std::string {
                if (!kSettings.mTime.mSync)
                    return "\n Not calibrated\n Set Date/Time\n then press Sync";
                auto& sync = *kSettings.mTime.mSync;
                tmElements_t last;
                breakTime(sync.mTime.tv_sec, last);
                char lastTime[32];
                std::sprintf(lastTime, "\n  %02d:%02d:%02d\n  %02d/%02d/%04d", last.Hour, last.Minute, last.Second, last.Day, last.Month, last.Year + 1970);
                auto elapsed = mTime.getTimeval().tv_sec - sync.mTime.tv_sec;
                auto actuallyElapsed = elapsed - kSettings.mTime.mSync->mDrift.tv_sec;
                bool megasec = actuallyElapsed > 1'000'000;
                char ppm[10];
                std::sprintf(ppm, "%+.2f", mTime.getPpm());

                return "\n Last sync on:" + std::string(lastTime) +
                "\n Err: " + std::to_string(kSettings.mTime.mSync->mDrift.tv_sec) +
                    "/" + std::to_string(actuallyElapsed / (megasec ? 1'000'000 : 1)) + (megasec ? "M" : "") +
                "\n PPM:" + std::string(ppm) +
                "\n Cal:" + std::to_string(kSettings.mTime.mCalibration);
            }},
        }},
        UI::Menu{"Hour Beep", {
            UI::Bool{"Beep",
                [](){return kSettings.mHourly.mBeep; },
                [](bool val){ kSettings.mHourly.mBeep = val; }
            },
            UI::Bool{"Vibrate",
                [](){return kSettings.mHourly.mVib; },
                [](bool val){ kSettings.mHourly.mVib = val; }
            },
            UI::Loop<int>{"St Hour",
                []() -> int { return kSettings.mHourly.mStart; },
                [](){ kSettings.mHourly.mStart = (kSettings.mHourly.mStart + 1) % 24; }
            },
            UI::Loop<int>{"End Hour",
                []() -> int { return kSettings.mHourly.mEnd; },
                [](){ kSettings.mHourly.mEnd = (kSettings.mHourly.mEnd + 1) % 24; }
            }
        }},
        UI::Menu{"Alarms", {
        }},
    }},
    UI::Menu{"Watchface", {
        UI::Loop<int>{"Style",
            []() -> int { return kSettings.mWatchface.mType; },
            [](){ kSettings.mWatchface.mType = (kSettings.mWatchface.mType + 1) % 4; }
        },
    }},
    UI::Menu{"Display", {
        UI::Bool{"Invert",
            [](){return kSettings.mDisplay.mInvert; },
            [](bool val){ kSettings.mDisplay.mInvert = val; }
        },
        UI::Bool{"Border",
            [](){return kSettings.mDisplay.mDarkBorder; },
            [](bool val){ kSettings.mDisplay.mDarkBorder = val; }
        },
        UI::Loop<int>{"Rotation",
            []() -> int { return kSettings.mDisplay.mRotation; },
            [](){ kSettings.mDisplay.mRotation = (kSettings.mDisplay.mRotation + 1) % 4; }
        },
    }},

    // UI::Menu{"Touch", {}},
    // UI::Menu{"Test", {
    //     UI::Action{"Vib 2x75ms", [&](){
    //         mDisplay.mTasks.push(std::packaged_task<void(void)>([](){
    //             Peripherals::vibrator(std::vector<int>{75,75,75});
    //         }));
    //     }},
    //     UI::Action{"Vib 1x75ms", [&](){
    //         mDisplay.mTasks.push(std::packaged_task<void(void)>([](){
    //             Peripherals::vibrator(std::vector<int>{75});
    //         }));
    //     }},
    //     UI::Action{"Vib 200ms", [&](){
    //         mDisplay.mTasks.push(std::packaged_task<void(void)>([](){
    //             Peripherals::vibrator(std::vector<int>{200});
    //         }));
    //     }},
    //     UI::Action{"Scale", [&](){
    //         mDisplay.mTasks.push(std::packaged_task<void(void)>([](){
    //             Peripherals::speaker(std::vector<std::pair<int,int>>{
    //                 {200,1000},{400,1000},{800,1000},{1600,1000},{3200,1000},
    //                 {6400,1000},{10000,1000},{12000,1000}});
    //         }));
    //     }},
    //     UI::Action{"Beep", [&](){
    //         mDisplay.mTasks.push(std::packaged_task<void(void)>([](){
    //             Peripherals::speaker(
    //                 std::vector<std::pair<int,int>>{
    //                 {3200,100},{0,100},{3200,100}});
    //         }));
    //     }},
    //     UI::Action{"Tetris", [&](){
    //         mDisplay.mTasks.push(std::packaged_task<void(void)>([](){
    //             Peripherals::tetris();
    //         }));
    //     }},
    // }},
    // UI::Menu{"Test2", {
    //     UI::Action{"Light 1s", [&](){
    //         mDisplay.mTasks.push(std::packaged_task<void(void)>([](){
    //             Peripherals::light(1'000'000);
    //         }));
    //     }},
    //     UI::Action{"Light 100s", [&](){
    //         mDisplay.mTasks.push(std::packaged_task<void(void)>([](){
    //             Peripherals::light(100'000'000);
    //         }));
    //     }},
    // }},
}}}
{
}

void Core::boot() {
    // ESP_LOGE("", "boot %lu", micros());

    mDisplay.init();

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
    if (mNow.Minute == 0
        && wakeup_reason == ESP_SLEEP_WAKEUP_TIMER
        && mNow.Hour >= kSettings.mHourly.mStart
        && mNow.Hour <= kSettings.mHourly.mEnd)
    {
        if (kSettings.mHourly.mBeep) {
            mDisplay.mTasks.push(std::packaged_task<void(void)>([](){
                Peripherals::speaker(
                    std::vector<std::pair<int,int>>{
                    {3200,100},{0,100},{3200,500}
                });
            }));
        }
        if (kSettings.mHourly.mVib) {
            mDisplay.mTasks.push(std::packaged_task<void(void)>([](){
                Peripherals::vibrator(std::vector<int>{75,75,75});
            }));
        }
    }

    // Common display preparations, post UI Events processing
    auto& disp = kSettings.mDisplay;
    mDisplay.setRotation(disp.mRotation);
    mDisplay.setDarkBorder(disp.mDarkBorder ^ disp.mInvert);
    mDisplay.setInverted(!disp.mInvert);
    mDisplay.setTextColor(0xFF);

    // Show watch face or menu ?
    if (kSettings.mUi.mDepth < 0) {
        #define ARGS kSettings.mDisplay, kSettings.mWatchface, mDisplay, mBattery, mNow
        // Instantiate the watchface type we are using
        switch(kSettings.mWatchface.mType) {
            default: DefaultWatchface(ARGS).draw(); break;
            // case 0: break;
            // case 1: break;
            // case 2: break;
            // case 3: break;
        }
        #undef ARGS
        Light::off(); // Always turn off light exiting the Menus
    } else {
        kSettings.mWatchface.mLastDraw.mValid = false;
        std::visit([&](auto& e){
            if constexpr (has_render<decltype(e), Display&>::value) {
                e.render(mDisplay);
            } else {
                mDisplay.setTextSize(3);
                mDisplay.setTextColor(1, 0);
                mDisplay.println("UNIMPLEMENTED");
                mDisplay.println("PRESS BACK");
            }
        }, findUi());
    }

    deepSleep();
}

void Core::firstTimeBoot() {
    // For some reason, seems to be enabled on first boot
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    // Select default voltage 2.6V
    Power::low();
    Light::off();
    // HACK: Set a fixed time to start with
    struct timeval tv{.tv_sec=1723194200, .tv_usec=0};
    // struct timezone tz{.tz_minuteswest=60, .tz_dsttime=1};
    mTime.setTime(tv);
    // reset calibration to the ESP32
    mTime.calReset();
}
const UI::Any& Core::findUi() {
    // Find current UI element in view by recursively finding deeper elements
    const UI::Any* item = &mUi;
    for (auto i=0; i < kSettings.mUi.mDepth; i++) {
        auto& index = kSettings.mUi.mState[i];
        std::visit([&](auto& e) {
            if constexpr (has_sub<decltype(e), uint8_t>::value) {
                item = &e.sub(index);
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
        if (btn == Touch::Light) {
            mDisplay.mTasks.push(std::packaged_task<void(void)>([](){
                Light::toggle();
            }));
        } else if (btn == Touch::Menu) {
            ui.mDepth = 0;
        }
        return;
    }

    // Button press on the UI is sent to the current selected item
    auto& item = findUi();

    // Send the touch event to the UI element to handle it, or fallback
    std::visit([&]<typename E>(const E& e) {
        if constexpr (has_button<E>::value) {
            e.button(btn);
        } else {
            // If a generic button handler is not implemented, try the specific ones
            switch (btn) {
            case Touch::Back: {
                if constexpr (has_button_back<E>::value) {
                    e.button_back();
                } else {
                    // Default option is to go back in the UI
                    ui.mDepth--;
                }
            } break;
            case Touch::Menu: {
                if constexpr (has_button_menu<E>::value) {
                    e.button_menu();
                }
            } break;
            default: {
                if constexpr (has_button_updown<E, int>::value) {
                    e.button_updown(btn == Touch::Up ? 1 : -1);
                }
            } break;
            }
        }
    }, item);
}

/*void Core::NTPSync() {
    // Select default voltage 2.9V/3.3V for WiFi
    Power::lock();
    //sleep(3);
    // We need Arduino for this (WiFi + NTP)
    initArduino();
    showSyncNTP();
    Power::unlock();
}*/

void Core::deepSleep() {
    mDisplay.hibernate();

    if (!kSettings.mLeakPinsSet) {
        kSettings.mLeakPinsSet = true;

        // Set oscilator config PCF8563
        // TODO

        // Can take 1ms to run this code
        // Set all GPIOs to input that we are not using to avoid leaking power
        // Set GPIOs 0-39 to input to avoid power leaking out
        const uint64_t ignore = 0b11110001000000110000100111000010; // Ignore some GPIOs due to resets
        for (int i = 0; i < GPIO_NUM_MAX; i++) {
            if ((ignore >> i) & 0b1)
                continue;
            // ESP_LOGE("", "%d input", i);
            pinMode(i, INPUT);
        }
    }

    mTouch.setUp(kSettings.mUi.mDepth < 0); // Takes 0.3ms -> 10uAs

    // ESP_LOGE("deepSleep", "%ld", micros());

    // Calculate stepsize based on battery level or on battery save mode
    auto stepSize = [&]() {
        if (mBattery.mCurPercent < 100 || mNow.Hour < 7) { // TODO: Proper power save night mode
            return 5;
        } else if (mBattery.mCurPercent < 200) {
            return 4;
        } else if (mBattery.mCurPercent < 500) {
            return 2;
        } else {
            return 1;
        }
    }();

    // TODO: When there is an alarm, we need to wake up earlier
    auto nextFullWake = 60;
    auto firstMinutesSleep = stepSize - mNow.Minute % stepSize;
    auto nextPartialWake = firstMinutesSleep + mNow.Minute;
    // In case the step overflows, we need to chop it, and wake up earlier
    // kDSState.minutes will be exactly 0 after this trim
    if (nextPartialWake > nextFullWake)
        firstMinutesSleep -= nextPartialWake - nextFullWake;

    // ESP_LOGE("", "nextFullWake %d firstMinutesSleep %d nextPartialWake %d", nextFullWake, firstMinutesSleep, nextPartialWake);

    // We can only run wakeupstub when on watchface mode
    if (kSettings.mUi.mDepth < 0) {
        kDSState.currentMinutes = mNow.Minute + firstMinutesSleep;
        kDSState.minutes = nextFullWake - mNow.Minute - firstMinutesSleep;
        // ESP_LOGE("", "min %d step %d wait %ld", kDSState.minutes, stepSize, kDSState.updateWait);
        kDSState.stepSize = stepSize;
        // Only trigger the wakeupstub if there is any minute left
        if (kDSState.minutes > 0) 
            esp_set_deep_sleep_wake_stub(&wake_stub_example);
    }

    auto nextMinute = (60 - mNow.Second) * 1'000'000 - mTime.getTimeval().tv_usec;
    esp_sleep_enable_timer_wakeup(nextMinute + (firstMinutesSleep - 1) * 60'000'000);
    esp_deep_sleep_start();
    ESP_LOGE("deepSleep", "never reach!");
}
