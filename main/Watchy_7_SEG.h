#ifndef WATCHY_7_SEG_H
#define WATCHY_7_SEG_H

#include "Watchy.h"
#include "Seven_Segment10pt7b.h"
#include "DSEG7_Classic_Regular_15.h"
#include "DSEG7_Classic_Bold_25.h"
#include "DSEG7_Classic_Regular_39.h"
#include "icons.h"

class Watchy7SEG : public Watchy{
    using Watchy::Watchy;
    public:
        void drawWatchFace();
        void drawTime();
        void drawDate();
        void drawSteps();
        void drawWeather();
        void drawBattery();

        void drawEllipse(int x, int y, uint8_t width, uint8_t height, uint16_t on);
        void drawMoonFast(float p, int x, int y, uint8_t r, uint16_t on, uint16_t off);
        void drawMoon(float p, uint16_t x, uint16_t y, uint16_t radius, uint16_t on, uint16_t off);
        void drawEllipseDifference(int x, int y, uint8_t width1, uint8_t width2, uint8_t height, bool big, uint16_t on);
        //void drawEllipseDifference(int x, int y, uint8_t width1, uint8_t height1, uint8_t width2, uint8_t height2, uint16_t on);
};

#endif