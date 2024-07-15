#pragma once

#include "display.h"
#include "battery.h"
#include <TimeLib.h>
#include "ui.h"

struct Rect {
    uint8_t x, y, w, h;
    uint16_t size() const {return w/8 * h; };
};

struct WatchfaceSettings {
    uint8_t mType : 2 {0}; // Watchface type selected from the presets

    // Draw cache, try to only draw once the hour
    // Also can be used to SPI transfer form deep_sleep wake stub
    struct {
        // Rotation&watchface, invalidates cache if changes
        bool mDone : 1 {false};
        uint8_t mRotation : 2 {2};
        uint8_t mType : 2 {0};

        struct Units {
            Rect coord {}; 
            uint8_t data[300 * 10]{};
        } mUnits;
        struct Decimal {
            Rect coord {};
            uint8_t data[300 * 6]{};
        } mDecimal;
    } mCache;

    // Store information about the last draw, the most important one is
    // if the draw was valid or not, there is 128 bytes for extra data for the watchfaces
    struct {
        bool mValid{false}; // If the last draw was a watchface
        uint8_t mMinuteU{}, mMinuteD{}; // The minutes are handled by base class
        uint8_t mStore[128]{}; // Scratch data the Watchfaces want to store
    } mLastDraw;
};

/* This class handles the Draw of the display watchface
 * in an optimal way, caching elements to avoid redrawing
 */
class Watchface {
protected:
    struct {
        DisplaySettings& mDisplay;
        WatchfaceSettings& mWatchface;
    } mSettings;
    Display& mDisplay;
    const Battery& mBattery;
    const tmElements_t& mNow;

    // Needs to implement minute uni/dec draw & return Rect coordinates
    virtual void drawU(uint8_t d);
    virtual void drawD(uint8_t d);
    virtual Rect rectU();
    virtual Rect rectD();

    // Can optionally implement Other element drawing based & return vect of rect
    virtual std::vector<Rect> render() { return {}; }

    constexpr static uint8_t mainColor = 0xFF;
    constexpr static uint8_t backColor = 0x0;

public:
    explicit Watchface(
        DisplaySettings& dispSet,
        WatchfaceSettings& watchSet,
        Display& display, 
        Battery& battery,
        const tmElements_t& now)
    : mSettings{dispSet, watchSet}
    , mDisplay{display}
    , mBattery(battery)
    , mNow(now)
    {}

    void draw();
    void updateCache();

    void copyRectToDisplay(const Rect& rect) {
      auto& [x, y, w, h] = rect;
      mDisplay.writeRegion(x, y, w, h);
    };
    void copyAlignedRectToDisplay(const Rect& rect) {
      auto& [x, y, w, h] = rect;
      mDisplay.writeRegionAligned(x, y, w, h);
    };
};