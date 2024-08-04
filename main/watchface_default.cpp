#include "watchface_default.h"

#include <Fonts/FreeMonoBold9pt7b.h>
#include "fonts/DSEG7_Classic_Bold_53.h"
#include "fonts/Seven_Segment10pt7b.h"
#include "fonts/DSEG7_Classic_Regular_15.h"
#include "fonts/DSEG7_Classic_Bold_25.h"
#include "fonts/DSEG7_Classic_Regular_39.h"
#include "icons.h"

std::vector<Rect> DefaultWatchface::render() {
  std::vector<Rect> rects;

  auto& last = mSettings.mWatchface.mLastDraw;
  const auto& redraw = !last.mValid;
  struct Store {
    uint8_t mHour{};
    struct Date {
      uint8_t mYear{}, mMonth{}, mDay{};
      auto operator<=>(const Date&) const = default;
    } mDate;
    uint16_t mBattery{};
  };
  Store& store = *reinterpret_cast<Store*>(last.mStore);
  assert(sizeof(Store) < sizeof(last.mStore));

  // Minute separator
  if (redraw) {
    mDisplay.setFont(&DSEG7_Classic_Bold_53);
    mDisplay.setCursor(92,73);
    mDisplay.print(':');
    rects.emplace_back(92, 0, 10, 53);
  }

  // Hour
  if (redraw || store.mHour != mNow.Hour) {
    store.mHour = mNow.Hour;
    mDisplay.setFont(&DSEG7_Classic_Bold_53);
    mDisplay.setCursor(4, 73);
    if(mNow.Hour < 10){
        mDisplay.print('0');
    }
    mDisplay.print(mNow.Hour);
    rects.emplace_back(4, 20, 80, 53);
  }

  // Date
  if (redraw || store.mDate != Store::Date{mNow.Year, mNow.Month, mNow.Day}) {
    store.mDate = Store::Date{mNow.Year, mNow.Month, mNow.Day};
    
    mDisplay.setFont(&Seven_Segment10pt7b);
    constexpr auto x = 17, y = 97;
    mDisplay.setCursor(x, y);

    String dayOfWeek = dayShortStr(mNow.Wday);
    mDisplay.setCursor(x, y);
    mDisplay.println(dayOfWeek);

    String month = monthShortStr(mNow.Month);
    mDisplay.setCursor(x + 70, y);
    mDisplay.println(month);

    mDisplay.setFont(&DSEG7_Classic_Regular_15);
    mDisplay.setCursor(x + 40, y+1);
    if(mNow.Day < 10){
        mDisplay.print('0');
    }
    mDisplay.println(mNow.Day);
    mDisplay.setCursor(x + 110, y+1);
    mDisplay.println(tmYearToCalendar(mNow.Year));// offset from 1970, since year is stored in uint8_t

    rects.emplace_back(0, 75, 200, 24);
  }

  // Battery
  if (redraw || store.mBattery != mBattery.mCurPercent) {
    store.mBattery = mBattery.mCurPercent;
    mDisplay.setFont(NULL);
    mDisplay.setCursor(68, 120);
    mDisplay.setTextSize(2);
    mDisplay.printf("%.1f%%", mBattery.mCurPercent * 0.1);
    rects.emplace_back(68, 110, 60, 30);
  }

  return rects;
}

void DefaultWatchface::drawD(uint8_t d){
  mDisplay.setFont(&DSEG7_Classic_Bold_53);
  mDisplay.setCursor(104, 73);
  mDisplay.println(d);
}

void DefaultWatchface::drawU(uint8_t d){
  mDisplay.setFont(&DSEG7_Classic_Bold_53);
  mDisplay.setCursor(148, 73);
  mDisplay.println(d);
}



// void DefaultWatchface::date(int16_t x, int16_t y){
//     mDisplay.setFont(&Seven_Segment10pt7b);

//     mDisplay.setCursor(x, y);

//     String dayOfWeek = dayShortStr(mNow.Wday);
//     mDisplay.setCursor(x, y);
//     mDisplay.println(dayOfWeek);

//     String month = monthShortStr(mNow.Month);
//     mDisplay.setCursor(x + 70, y);
//     mDisplay.println(month);

//     mDisplay.setFont(&DSEG7_Classic_Regular_15);
//     mDisplay.setCursor(x + 40, y+1);
//     if(mNow.Day < 10){
//         mDisplay.print("0");
//     }
//     mDisplay.println(mNow.Day);
//     mDisplay.setCursor(x + 110, y+1);
//     mDisplay.println(tmYearToCalendar(mNow.Year));// offset from 1970, since year is stored in uint8_t
// }

// void DefaultWatchface::batteryIcon(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
//     mDisplay.drawRect(x + 2, y + 0, w - 4, h - 0, color);
//     mDisplay.drawRect(x + 1, y + 1, w - 2, h - 2, color);
//     mDisplay.drawRect(x + 0, y + 2, w - 0, h - 4, color);
//     // Pointy end
//     // mDisplay.drawRect(x + w, y + 4, 2, h - 8, color);
// }
// void DefaultWatchface::battery(int16_t x, int16_t y) {
//     //mDisplay.drawBitmap(154, 73, battery, 37, 21, mainColor);
//     batteryIcon(140, 73, 55, 23, mainColor);

//     mDisplay.setTextSize(2);
//     mDisplay.setFont(NULL);
//     // mDisplay.setFont(&FreeMonoBold9pt7b);
//     // mDisplay.setFont(&Seven_Segment10pt7b);

//     mDisplay.setCursor(142, 77);
//     float perc = mBattery.mCurPercent;
//     mDisplay.printf("%.1f", perc+90);
//     //display.fillRect(159, 78, 27, BATTERY_SEGMENT_HEIGHT, mainColor);//clear battery segments
//     // if(VBAT > 4.1){
//     //     batteryLevel = 3;
//     // }
//     // else if(VBAT > 3.95 && VBAT <= 4.1){
//     //     batteryLevel = 2;
//     // }
//     // else if(VBAT > 3.80 && VBAT <= 3.95){
//     //     batteryLevel = 1;
//     // }
//     // else if(VBAT <= 3.80){
//     //     batteryLevel = 0;
//     // }

//     // for(int8_t batterySegments = 0; batterySegments < batteryLevel; batterySegments++){
//     //     display.fillRect(159 + (batterySegments * BATTERY_SEGMENT_SPACING), 78, BATTERY_SEGMENT_WIDTH, BATTERY_SEGMENT_HEIGHT, mainColor);
//     // }
// }