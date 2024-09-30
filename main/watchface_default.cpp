#include "watchface_default.h"

#include <Fonts/FreeMonoBold9pt7b.h>
#include "fonts/DSEG7_Classic_Bold_53.h"
#include "fonts/Seven_Segment10pt7b.h"
#include "fonts/DSEG7_Classic_Regular_15.h"
#include "fonts/DSEG7_Classic_Bold_25.h"
#include "fonts/DSEG7_Classic_Regular_39.h"
#include "icons.h"

double JulianDateFromUnixTime(uint64_t t){
	//Not valid for dates before Oct 15, 1582
	return (t / 86'400.0) + 2440587.5;
}

double mod360(double d){
    double t = std::fmod(d, 360.0);
    if(t<0){t+=360;}
    return t;
}

double getIlluminatedFractionOfMoon(uint64_t t){
    const double jd = JulianDateFromUnixTime(t);
    const double toRad = PI/180.0;
    const double T=(jd-2451545)/36525.0;

    const auto D = mod360(297.8501921 + 445267.1114034*T - 0.0018819*T*T + 1.0/545868.0*T*T*T - 1.0/113065000.0*T*T*T*T)*toRad; //47.2
    const auto M = mod360(357.5291092 + 35999.0502909*T - 0.0001536*T*T + 1.0/24490000.0*T*T*T)*toRad; //47.3
    const auto Mp = mod360(134.9633964 + 477198.8675055*T + 0.0087414*T*T + 1.0/69699.0*T*T*T - 1.0/14712000.0*T*T*T*T)*toRad; //47.4

    // const auto i=mod360(0 - D*180/PI - 6.289 * sin(Mp) + 2.1 * sin(M) -1.274 * sin(2*D - Mp) -0.658 * sin(2*D) -0.214 * sin(2*Mp) -0.11 * sin(D))*toRad;

    // const auto k=(1 + cos(i))/2;
    // const auto r = std::fmod(-i / PI / 2 * 100 + 100, 100.f);

    //48.4
    const auto i=mod360(180 - D*180/PI - 6.289 * sin(Mp) + 2.1 * sin(M) -1.274 * sin(2*D - Mp) -0.658 * sin(2*D) -0.214 * sin(2*Mp) -0.11 * sin(D))*toRad;

    const auto k=(1 + cos(i))/2;
    if (i > PI)
      return 100-k*50;
    // ESP_LOGE("moon", "r %f, i %f k %f, t %lld, jd %f", r, i, k, t, jd);
    return k * 50;
}

std::vector<Rect> DefaultWatchface::render() {
  std::vector<Rect> rects;

  auto& config = mSettings.mWatchface.mConfig;

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
    rects.emplace_back(92, 30, 12, 35);
  }

  // Hour / Moon
  if (redraw || store.mHour != mNow.Hour) {
    store.mHour = mNow.Hour;
    mDisplay.setFont(&DSEG7_Classic_Bold_53);
    mDisplay.setCursor(4, 73);
    mDisplay.printf("%02d", mNow.Hour);
    rects.emplace_back(7, 20, 80, 53);

    // Moon
    if (config.mMoon) {
      drawMoon(getIlluminatedFractionOfMoon(mTime.getTimeval().tv_sec), 50, 150, 30, 1, 0);
      rects.emplace_back(15, 115, 70, 70);
    }
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

    rects.emplace_back(0, 80, 200, 20);
  }

  // Battery
  if (config.mBattery && (redraw || store.mBattery != mBattery.mCurPercent)) {
    store.mBattery = mBattery.mCurPercent;
    mDisplay.setFont(NULL);
    mDisplay.setCursor(85, 105);
    mDisplay.setTextSize(1);
    mDisplay.printf("%.1f%%", mBattery.mCurPercent * 0.1);
    rects.emplace_back(75, 102, 45, 14);
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



std::vector<std::pair<uint8_t, uint8_t>> calcEllipse(int x, int y, uint8_t width, uint8_t height)
{
    int x1 = -width, y1 = 0; // II quadrant from bottom left to top right
    int e2 = height, dx = (1 + 2 * x1) * e2 * e2; // error increment
    int dy = x1 * x1, err = dx + dy; // error of 1 step

    std::vector<std::pair<uint8_t, uint8_t>> lines;
    lines.reserve(height);

    do {
        e2 = 2 * err;

        if (e2 >= dx) {
            x1++;
            err += dx += 2 * height * height;
        } // x1 step

        if (e2 <= dy) {
            // Whenever we advance to next line we print the previous line
            lines.emplace_back(y1, -x1);
            y1++;
            err += dy += 2 * width * width;
        } // y1 step
    } while (x1 <= 0);

    // -> finish tip of ellipse ?
    // display.drawLine(x, y - y1, x, y + y1, on);
    return lines;
}

void DefaultWatchface::drawEllipse(int x, int y, uint8_t width, uint8_t height, uint16_t on)
{
    for (auto& [y1, x1] : calcEllipse(x, y, width, height)) {
        mDisplay.drawLine(x + x1, y - y1, x - x1, y - y1, on);
        mDisplay.drawLine(x + x1, y + y1, x - x1, y + y1, on);
    }
}

void DefaultWatchface::drawEllipseDifference(int x, int y, uint8_t width1, uint8_t width2, uint8_t height, bool big, uint16_t color)
{
    auto elipse1 = calcEllipse(x, y, width1, height);
    auto elipse2 = calcEllipse(x, y, width2, height);

    for (auto i = 0; i < elipse1.size(); i++) {
        auto& [y11, x11] = elipse1[i];
        auto& [y12, x12] = elipse2[i];
        // ESP_LOGE("", "%d %d %d %d", y11, y12, x11, x12);
        if (big) {
            mDisplay.drawLine(x - x11, y - y11, x + x12, y - y11, color);
            mDisplay.drawLine(x - x11, y + y11, x + x12, y + y11, color);
        } else {
            mDisplay.drawLine(x + x11, y - y11, x + x12, y - y11, color);
            mDisplay.drawLine(x + x11, y + y11, x + x12, y + y11, color);
        }
    }
}

void DefaultWatchface::drawMoonFast(float p, int x, int y, uint8_t r, uint16_t on, uint16_t off)
{
  if (p < 25)
    drawEllipseDifference(x, y, r * (25 - p) / 25, r, r, false, off);
  else if (p < 50)
    drawEllipseDifference(x, y, r * (p - 25) / 25, r, r, true, off);
  else if (p < 75)
    drawEllipseDifference(x, y, r, r * (75 - p) / 25, r, true, off);
  else
    drawEllipseDifference(x, y, r, r * (p - 75) / 25, r, false, off);
}

void DefaultWatchface::drawMoon(float p, uint16_t x, uint16_t y, uint16_t radius, uint16_t on, uint16_t off) 
{
  mDisplay.drawCircle(x, y, radius + 2, on);
  mDisplay.drawCircle(x, y, radius + 3, off);

  //drawEllipseDifference(x, y, radius * (25 - p) / 25, radius, radius, radius, on);
  
  mDisplay.startWrite();
  mDisplay.fillCircleHelper(x, y, radius, p < 50 ? 2 : 1, 0, on);
  mDisplay.fillCircleHelper(x, y, radius, p < 50 ? 1 : 2, 0, off);
  mDisplay.endWrite();
  
  if (p < 25)
    drawEllipse(x, y, radius * (25 - p) / 25, radius, on);
  else if (p < 50)
    drawEllipse(x, y, radius * (p - 25) / 25, radius, off);
  else if (p < 75)
    drawEllipse(x, y, radius * (75 - p) / 25, radius, off);
  else
    drawEllipse(x, y, radius * (p - 75) / 25, radius, on);
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