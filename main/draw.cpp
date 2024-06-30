#include "draw.h"

void Draw::initialize() {
  mDisplay.setRotation(mSettings.mRotation);
  // mDisplay.epd2.setDarkBorder(mSettings.mDarkBorder ^ mSettings.mInvert);
  // mDisplay.epd2.initDisplay();
  mDisplay.fillScreen(backColor());
  mDisplay.setTextColor(mainColor());
}

uint8_t Draw::mainColor() const {
    return mSettings.mInvert ? 0xFF : 0x00; 
}
uint8_t Draw::backColor() const {
    return mSettings.mInvert ? 0x00 : 0xFF; 
}

#include <Fonts/FreeMonoBold9pt7b.h>
#include "DSEG7_Classic_Bold_53.h"
#include "Seven_Segment10pt7b.h"
#include "DSEG7_Classic_Regular_15.h"
#include "DSEG7_Classic_Bold_25.h"
#include "DSEG7_Classic_Regular_39.h"
#include "icons.h"

void Draw::time_hour(int16_t x, int16_t y){
    mDisplay.setFont(&DSEG7_Classic_Bold_53);
    mDisplay.setCursor(x, y);
    if(mNow.Hour < 10){
        mDisplay.print("0");
    }
    mDisplay.print(mNow.Hour);
}

void Draw::time_mid(int16_t x, int16_t y){
    mDisplay.setFont(&DSEG7_Classic_Bold_53);
    mDisplay.setCursor(x, y);
    mDisplay.print(":");
}

void Draw::time_min_10(int16_t x, int16_t y){
    mDisplay.setFont(&DSEG7_Classic_Bold_53);
    mDisplay.setCursor(x, y);
    if(mNow.Minute < 10){
        mDisplay.print("0");
    }
    mDisplay.println(mNow.Minute / 10);
}

void Draw::time_min_01(int16_t x, int16_t y){
    mDisplay.setFont(&DSEG7_Classic_Bold_53);
    mDisplay.setCursor(x, y);
    mDisplay.println(mNow.Minute % 10);
}

void Draw::date(int16_t x, int16_t y){
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

void Draw::watchFace() {
    initialize();

    auto& last = mSettings.mLastDraw;

    auto copyImageToDisplay = [&](uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
      /*x = mDisplay.gx_uint16_min(x, mDisplay.width());
      y = mDisplay.gx_uint16_min(y, mDisplay.height());
      w = mDisplay.gx_uint16_min(w, mDisplay.width() - x);
      h = mDisplay.gx_uint16_min(h, mDisplay.height() - y);
      mDisplay._rotate(x, y, w, h);
      mDisplay.epd2.writeImagePart(mDisplay._buffer, x, y, 200, 200, x, y, w, h);*/
      mDisplay._rotate(x, y, w, h);
      mDisplay.writeRegion(x, y, x, y, w, h, false, false);
    };


    // Create a list of elements with the conditions to trigger them and how to compose
    struct Composable {
      bool compose;
      std::array<uint8_t, 4> rect;
      std::function<void(void)> func;
    };
    std::array<Composable, 5> composables{{
      {
        last.mTime.Hour != mNow.Hour,
        {{4, 20, 80, 53}},
        [&](){time_hour(4, 73);} // 2ms cost
      },
      {
        false,
        {{92, 0, 10, 53}},
        [&](){time_mid(92, 73);}
      },
      {
        last.mTime.Minute / 10 != mNow.Minute / 10,
        {{104, 20, 40, 53}},
        [&](){time_min_10(104, 73);} // 1ms cost
      },
      {
        last.mTime.Minute != mNow.Minute,
        {{148, 20, 40, 53}},
        [&](){time_min_01(148, 73);} // 1ms cost
      },
      {
        last.mTime.Year != mNow.Year || last.mTime.Month != mNow.Month || last.mTime.Day != mNow.Day,
        {{0, 75, 200, 24}},
        [&](){date(17, 97);} // 1ms cost, rarely updates, so better in one block
      // },
      // {
      //   last.mBatery != mBattery.mCurPercent,
      //   {{68, 110, 100, 30}},
      //   [&](){
      //     mDisplay.setCursor(68, 120);
      //     mDisplay.setFont(NULL);
      //     mDisplay.setTextSize(2);
      //     mDisplay.printf("%.1f%%", mBattery.mCurPercent * 0.1);
      //   }
      }
    }};
    if (!last.mValid)
      for (auto& c : composables)
        c.compose = true;

    // Pass 1, render them + copy
    for (auto& c : composables) {
      if (c.compose) {
        c.func();
        if (last.mValid) // Only copy if it was valid, otherwise we will copy later the full buffer
          copyImageToDisplay(c.rect[0], c.rect[1], c.rect[2], c.rect[3]);
      }
    }
    // Pass 2, update display
    if (!last.mValid){
      // mDisplay.display(!mDisplay.epd2.displayFullInit);
      mDisplay.writeAllAndRefresh(!mDisplay.displayFullInit);
      mDisplay.writeAll(); //This is to get ready for partial updates
    } else {
      // Manual refresh + swap buffers
      // mDisplay.epd2.refresh(!mDisplay.epd2.displayFullInit);
      mDisplay.refresh(!mDisplay.displayFullInit);

      // Pass 3, copy again the updated parts to the other framebuffer
      for (auto& c : composables) {
        if (c.compose) {
          copyImageToDisplay(c.rect[0], c.rect[1], c.rect[2], c.rect[3]);
        }
      }
    }

    // Store the values for next round
    last.mValid = true;
    last.mTime = mNow;
    last.mBatery = mBattery.mCurPercent;
}
void Draw::batteryIcon(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    mDisplay.drawRect(x + 2, y + 0, w - 4, h - 0, color);
    mDisplay.drawRect(x + 1, y + 1, w - 2, h - 2, color);
    mDisplay.drawRect(x + 0, y + 2, w - 0, h - 4, color);
    // Pointy end
    // mDisplay.drawRect(x + w, y + 4, 2, h - 8, color);
}
void Draw::battery(int16_t x, int16_t y) {
    //mDisplay.drawBitmap(154, 73, battery, 37, 21, mainColor());
    batteryIcon(140, 73, 55, 23, mainColor());

    mDisplay.setTextSize(2);
    mDisplay.setFont(NULL);
    // mDisplay.setFont(&FreeMonoBold9pt7b);
    // mDisplay.setFont(&Seven_Segment10pt7b);

    mDisplay.setCursor(142, 77);
    float perc = mBattery.mCurPercent;
    mDisplay.printf("%.1f", perc+90);
    //display.fillRect(159, 78, 27, BATTERY_SEGMENT_HEIGHT, mainColor());//clear battery segments
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
    //     display.fillRect(159 + (batterySegments * BATTERY_SEGMENT_SPACING), 78, BATTERY_SEGMENT_WIDTH, BATTERY_SEGMENT_HEIGHT, mainColor());
    // }
}


void Draw::menu(const AnyItem& item, const uint8_t index) {
    initialize();
    mSettings.mLastDraw.mValid = false;

    // Text size for all the UI
    mDisplay.setTextSize(3);

    std::visit([&](auto&& e){
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, MenuItem>) {
            // Print the menu title on top, centered
            mDisplay.setTextColor(mainColor(), backColor());
            mDisplay.print(" ");
            mDisplay.println(e.name);
            //mDisplay.setCursor(mDisplay.getCursorX(), mDisplay.getCursorY() + 5);
            for(auto i = 0;i < e.items.size(); i++) {
                if (i == index) {
                    mDisplay.setTextColor(backColor(), mainColor());
                } else {
                    mDisplay.setTextColor(mainColor(), backColor());
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
            mDisplay.setTextColor(mainColor(), backColor());
            mDisplay.println(e.get());
        }
    }, item);

    // mDisplay.display(true);
    mDisplay.writeAllAndRefresh(true);
}