// Display Library for SPI e-paper panels from Dalian Good Display and boards from Waveshare.
// Requires HW SPI and Adafruit_GFX. Caution: the e-paper panels require 3.3V supply AND data lines!
//
// based on Demo Example from Good Display, available here: http://www.e-paper-display.com/download_detail/downloadsId=806.html
// Panel: GDEH0154D67 : http://www.e-paper-display.com/products_detail/productId=455.html
// Controller : SSD1681 : http://www.e-paper-display.com/download_detail/downloadsId=825.html
//
// Author: Jean-Marc Zingg
//
// Version: see library.properties
//
// Library: https://github.com/ZinggJM/GxEPD2
//
// The original code from the author has been slightly modified to improve the performance for Watchy Project:
// Link: https://github.com/sqfmi/Watchy

#pragma once

#include <GxEPD2_EPD.h>
#include <GxEPD2_BW.h>

#include "hardware.h"


class Display1 : public Adafruit_GFX {
public:
  static constexpr uint8_t WIDTH = 200;
  static constexpr uint8_t HEIGHT = WIDTH;

  static constexpr bool kReduceBoosterTime = true; // Saves ~200ms + Reduce power usage
  static constexpr bool kFastUpdateTemp = true; // Saves 5ms + FixedSpeedier LUT (300ms update)
  
  bool darkBorder = false; // adds a dark border outside the normal screen area
  static RTC_DATA_ATTR bool displayFullInit; // Remembers when full init is required
  uint8_t buffer[WIDTH * HEIGHT / 8];

  SPISettings _spi_settings{20000000, MSBFIRST, SPI_MODE0};

  Display1();

  template <typename T> static inline void
  _swap_(T & a, T & b)
  {
    T t = a;
    a = b;
    b = t;
  };
  void _rotate(uint8_t& x, uint8_t& y, uint8_t& w, uint8_t& h)
  {
    switch (getRotation())
    {
      case 1:
        _swap_(x, y);
        _swap_(w, h);
        x = WIDTH - x - w;
        break;
      case 2:
        x = WIDTH - x - w;
        y = HEIGHT - y - h;
        break;
      case 3:
        _swap_(x, y);
        _swap_(w, h);
        y = HEIGHT - y - h;
        break;
    }
  }

  void drawPixel(int16_t x, int16_t y, uint16_t color)
    {
      if ((x < 0) || (x >= width()) || (y < 0) || (y >= height())) return;
      // check rotation, move pixel around if necessary
      switch (getRotation())
      {
        case 1:
          _swap_(x, y);
          x = WIDTH - x - 1;
          break;
        case 2:
          x = WIDTH - x - 1;
          y = HEIGHT - y - 1;
          break;
        case 3:
          _swap_(x, y);
          y = HEIGHT - y - 1;
          break;
      }
      uint16_t i = x / 8 + y * 25;// * (_pw_w / 8);
      if (color)
        buffer[i] = (buffer[i] | (1 << (7 - x % 8)));
      else
        buffer[i] = (buffer[i] & (0xFF ^ (1 << (7 - x % 8))));
    }
  void fillScreen(uint16_t color) // 0x0 black, >0x0 white, to buffer
  {
    memset(buffer, (color == GxEPD_BLACK) ? 0x00 : 0xFF, sizeof(buffer));
  }

  void setDarkBorder(bool darkBorder);

  void setRamArea(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
  void refresh(bool partial = true);
  void hibernate();
  void waitWhileBusy();
  void writeRegion(uint8_t x_part, uint8_t y_part, uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool invert = false, bool mirror_y = false);
  void writeAll(bool backBuffer = false);
  void writeAllAndRefresh(bool partial = true){
    writeAll();
    refresh(partial);
  }

  // SPI transfer methods
  void _startTransfer();
  void _transfer(uint8_t value);
  void _transfer(const uint8_t* value, size_t size);
  void _transferCommand(uint8_t c);
  void _endTransfer();
};

class Display : public GxEPD2_EPD
{
  public:
    // attributes
    static constexpr uint16_t WIDTH = 200;
    static constexpr uint16_t WIDTH_VISIBLE = WIDTH;
    static constexpr uint16_t HEIGHT = 200;
    static constexpr GxEPD2::Panel panel = GxEPD2::GDEH0154D67;
    static constexpr bool hasColor = false;
    static constexpr bool hasPartialUpdate = true;
    static constexpr bool hasFastPartialUpdate = true; // We need to copy to both buffers
    static constexpr uint16_t power_on_time = 100; // ms, e.g. 95583us
    static constexpr uint16_t power_off_time = 150; // ms, e.g. 140621us
    static constexpr uint16_t full_refresh_time = 2600; // ms, e.g. 2509602us
    static constexpr uint16_t partial_refresh_time = 500; // ms, e.g. 457282us

    static constexpr bool kReduceBoosterTime = true; // Saves ~200ms + Reduce power usage
    static constexpr bool kFastUpdateTemp = true; // Saves 5ms + FixedSpeedier LUT (300ms update)
    
    bool darkBorder = false; // adds a dark border outside the normal screen area
    static RTC_DATA_ATTR bool displayFullInit; // Remembers when full init is required

    // constructor
    Display();
    void initDisplay();
    void setDarkBorder(bool darkBorder);
    static void busyCallback(const void *);
    // methods (virtual)
    //  Support for Bitmaps (Sprites) to Controller Buffer and to Screen
    void clearScreen(uint8_t value = 0xFF); // init controller memory and screen (default white)
    void writeScreenBuffer(uint8_t value = 0xFF); // init controller memory (default white)
    void writeScreenBufferAgain(uint8_t value = 0xFF); // init previous buffer controller memory (default white)
    // write to controller memory, without screen refresh; x and w should be multiple of 8
    void writeImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void writeImageForFullRefresh(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void writeImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                        int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void writeImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void writeImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                        int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    // for differential update: set current and previous buffers equal (for fast partial update to work correctly)
    void writeImageAgain(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void writeImagePartAgain(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                             int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    // write sprite of native data to controller memory, without screen refresh; x and w should be multiple of 8
    void writeNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    // write to controller memory, with screen refresh; x and w should be multiple of 8
    void drawImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void drawImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                       int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void drawImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void drawImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                       int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    // write sprite of native data to controller memory, with screen refresh; x and w should be multiple of 8
    void drawNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void refresh(bool partial_update_mode = false); // screen refresh from controller memory to full screen
    void refresh(int16_t x, int16_t y, int16_t w, int16_t h); // screen refresh from controller memory, partial screen
    void powerOff(); // turns off generation of panel driving voltages, avoids screen fading over time
    void hibernate(); // turns powerOff() and sets controller to deep sleep for minimum power use, ONLY if wakeable by RST (rst >= 0)

  private:
    void _writeScreenBuffer(uint8_t command, uint8_t value);
    void _writeImage(uint8_t command, const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void _writeImagePart(uint8_t command, const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                         int16_t x, int16_t y, int16_t w, int16_t h, bool invert = false, bool mirror_y = false, bool pgm = false);
    void _setPartialRamArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void _PowerOn();
    void _PowerOff();
    void _InitDisplay();
    void _Init_Full();
    void _Init_Part();
    void _Update_Full();
    void _Update_Part();

    virtual void _reset();

    void _transferCommand(uint8_t command);
    void _transferLUT();
};

using DisplayBW = GxEPD2_BW<Display, Display::HEIGHT>;