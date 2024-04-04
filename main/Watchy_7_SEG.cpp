#include "Watchy_7_SEG.h"

#define DARKMODE false

const uint8_t BATTERY_SEGMENT_WIDTH = 7;
const uint8_t BATTERY_SEGMENT_HEIGHT = 11;
const uint8_t BATTERY_SEGMENT_SPACING = 9;
const uint8_t WEATHER_ICON_WIDTH = 48;
const uint8_t WEATHER_ICON_HEIGHT = 32;


#include <vector>

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

void Watchy7SEG::drawEllipse(int x, int y, uint8_t width, uint8_t height, uint16_t on)
{
    for (auto& [y1, x1] : calcEllipse(x, y, width, height)) {
        display.drawLine(x + x1, y - y1, x - x1, y - y1, on);
        display.drawLine(x + x1, y + y1, x - x1, y + y1, on);
    }
}

void Watchy7SEG::drawEllipseDifference(int x, int y, uint8_t width1, uint8_t width2, uint8_t height, bool big, uint16_t color)
{
    auto elipse1 = calcEllipse(x, y, width1, height);
    auto elipse2 = calcEllipse(x, y, width2, height);

    for (auto i = 0; i < elipse1.size(); i++) {
        auto& [y11, x11] = elipse1[i];
        auto& [y12, x12] = elipse2[i];
        // ESP_LOGE("", "%d %d %d %d", y11, y12, x11, x12);
        if (big) {
            display.drawLine(x - x11, y - y11, x + x12, y - y11, color);
            display.drawLine(x - x11, y + y11, x + x12, y + y11, color);
        } else {
            display.drawLine(x + x11, y - y11, x + x12, y - y11, color);
            display.drawLine(x + x11, y + y11, x + x12, y + y11, color);
        }
    }
}

void Watchy7SEG::drawMoonFast(float p, int x, int y, uint8_t r, uint16_t on, uint16_t off)
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

void Watchy7SEG::drawMoon(float p, uint16_t x, uint16_t y, uint16_t radius, uint16_t on, uint16_t off) 
{
  display.drawCircle(x, y, radius + 2, on);
  display.drawCircle(x, y, radius + 3, off);

  //drawEllipseDifference(x, y, radius * (25 - p) / 25, radius, radius, radius, on);
  
  display.startWrite();
  display.fillCircleHelper(x, y, radius, p < 50 ? 2 : 1, 0, on);
  display.fillCircleHelper(x, y, radius, p < 50 ? 1 : 2, 0, off);
  display.endWrite();
  
  if (p < 25)
    drawEllipse(x, y, radius * (25 - p) / 25, radius, on);
  else if (p < 50)
    drawEllipse(x, y, radius * (p - 25) / 25, radius, off);
  else if (p < 75)
    drawEllipse(x, y, radius * (75 - p) / 25, radius, off);
  else
    drawEllipse(x, y, radius * (p - 75) / 25, radius, on);
}

#include "driver/touch_sensor.h"

void Watchy7SEG::drawWatchFace(){
    display.fillScreen(DARKMODE ? GxEPD_BLACK : GxEPD_WHITE);
    display.setTextColor(DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    drawTime();
    drawDate();
    // drawSteps();
    // drawWeather();
    // drawBattery();
    // display.drawBitmap(120, 77, WIFI_CONFIGURED ? wifi : wifioff, 26, 18, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    // if(BLE_CONFIGURED){
    //     display.drawBitmap(100, 75, bluetooth, 13, 21, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    // }
    display.print(getBatteryVoltage());
    display.drawCircle(150, 100, 28, GxEPD_BLACK);
    display.fillCircle(150, 100, 25, GxEPD_BLACK);

    switch(esp_sleep_get_touchpad_wakeup_status()) {
        case 0:
            display.drawRect(190, 10, 10, 90, GxEPD_BLACK); break;
        case 2:
            display.drawRect(0, 10, 10, 90, GxEPD_BLACK); break;
        case 5:
            display.drawRect(0, 110, 10, 90, GxEPD_BLACK); break;
        case 6:
            display.drawRect(190, 110, 10, 90, GxEPD_BLACK); break;
        default:
        ;
    }
    //display.fillCircleHelper(150, 100, 25, 0b0011, 10, GxEPD_WHITE);

    // auto a = esp_timer_get_time();
    //drawMoon(fmod((currentTime.Second + currentTime.Minute*60.0f), 100), 160, 160, 30, GxEPD_WHITE, GxEPD_BLACK);
    //drawMoon(13, 160, 160, 30, GxEPD_WHITE, GxEPD_BLACK);
    // drawMoonFast(fmod((currentTime.Second + currentTime.Minute*60.0f), 100), 160, 160, 30, GxEPD_WHITE, GxEPD_BLACK);
    //drawMoonFast(13, 160, 160, 30, GxEPD_WHITE, GxEPD_BLACK);
    // auto b = esp_timer_get_time();
    // ESP_LOGE("", "%lld", b - a);
}

void Watchy7SEG::drawTime(){
    display.setFont(&DSEG7_Classic_Bold_53);
    display.setCursor(5, 53+5);
    int displayHour;
    if(HOUR_12_24==12){
      displayHour = ((currentTime.Hour+11)%12)+1;
    } else {
      displayHour = currentTime.Hour;
    }
    if(displayHour < 10){
        display.print("0");
    }
    display.print(displayHour);
    display.print(":");
    if(currentTime.Minute < 10){
        display.print("0");
    }
    display.println(currentTime.Minute);
}

void Watchy7SEG::drawDate(){
    display.setFont(&Seven_Segment10pt7b);

    int16_t  x1, y1;
    uint16_t w, h;

    String dayOfWeek = dayStr(currentTime.Wday);
    display.getTextBounds(dayOfWeek, 5, 85, &x1, &y1, &w, &h);
    if(currentTime.Wday == 4){
        w = w - 5;
    }
    display.setCursor(85 - w, 85);
    display.println(dayOfWeek);

    String month = monthShortStr(currentTime.Month);
    display.getTextBounds(month, 60, 110, &x1, &y1, &w, &h);
    display.setCursor(85 - w, 110);
    display.println(month);

    display.setFont(&DSEG7_Classic_Bold_25);
    display.setCursor(5, 120);
    if(currentTime.Day < 10){
    display.print("0");
    }
    display.println(currentTime.Day);
    display.setCursor(5, 150);
    display.println(tmYearToCalendar(currentTime.Year));// offset from 1970, since year is stored in uint8_t
}
void Watchy7SEG::drawSteps(){
    // reset step counter at midnight
    // if (currentTime.Hour == 0 && currentTime.Minute == 0){
    //   sensor.resetStepCounter();
    // }
    // uint32_t stepCount = sensor.getCounter();
    display.drawBitmap(10, 165, steps, 19, 23, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    display.setCursor(35, 190);
    // display.println(stepCount);
}
void Watchy7SEG::drawBattery(){
    display.drawBitmap(154, 73, battery, 37, 21, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    display.fillRect(159, 78, 27, BATTERY_SEGMENT_HEIGHT, DARKMODE ? GxEPD_BLACK : GxEPD_WHITE);//clear battery segments
    int8_t batteryLevel = 0;
    float VBAT = getBatteryVoltage();
    if(VBAT > 4.1){
        batteryLevel = 3;
    }
    else if(VBAT > 3.95 && VBAT <= 4.1){
        batteryLevel = 2;
    }
    else if(VBAT > 3.80 && VBAT <= 3.95){
        batteryLevel = 1;
    }
    else if(VBAT <= 3.80){
        batteryLevel = 0;
    }

    for(int8_t batterySegments = 0; batterySegments < batteryLevel; batterySegments++){
        display.fillRect(159 + (batterySegments * BATTERY_SEGMENT_SPACING), 78, BATTERY_SEGMENT_WIDTH, BATTERY_SEGMENT_HEIGHT, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    }
}

void Watchy7SEG::drawWeather(){

    weatherData currentWeather = getWeatherData();

    int8_t temperature = currentWeather.temperature;
    int16_t weatherConditionCode = currentWeather.weatherConditionCode;

    display.setFont(&DSEG7_Classic_Regular_39);
    int16_t  x1, y1;
    uint16_t w, h;
    display.getTextBounds(String(temperature), 0, 0, &x1, &y1, &w, &h);
    if(159 - w - x1 > 87){
        display.setCursor(159 - w - x1, 150);
    }else{
        display.setFont(&DSEG7_Classic_Bold_25);
        display.getTextBounds(String(temperature), 0, 0, &x1, &y1, &w, &h);
        display.setCursor(159 - w - x1, 136);
    }
    display.println(temperature);
    display.drawBitmap(165, 110, currentWeather.isMetric ? celsius : fahrenheit, 26, 20, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    const unsigned char* weatherIcon;

    //https://openweathermap.org/weather-conditions
    if(weatherConditionCode > 801){//Cloudy
    weatherIcon = cloudy;
    }else if(weatherConditionCode == 801){//Few Clouds
    weatherIcon = cloudsun;
    }else if(weatherConditionCode == 800){//Clear
    weatherIcon = sunny;
    }else if(weatherConditionCode >=700){//Atmosphere
    weatherIcon = atmosphere;
    }else if(weatherConditionCode >=600){//Snow
    weatherIcon = snow;
    }else if(weatherConditionCode >=500){//Rain
    weatherIcon = rain;
    }else if(weatherConditionCode >=300){//Drizzle
    weatherIcon = drizzle;
    }else if(weatherConditionCode >=200){//Thunderstorm
    weatherIcon = thunderstorm;
    }else
    return;
    display.drawBitmap(145, 158, weatherIcon, WEATHER_ICON_WIDTH, WEATHER_ICON_HEIGHT, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
}
