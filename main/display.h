// based on Demo Example from Good Display, available here: http://www.e-paper-display.com/download_detail/downloadsId=806.html
// Panel: GDEH0154D67 : http://www.e-paper-display.com/products_detail/productId=455.html
// Controller : SSD1681 : http://www.e-paper-display.com/download_detail/downloadsId=825.html
//
// Inspired on GxEPD2 by Author: Jean-Marc Zingg
// Library: https://github.com/ZinggJM/GxEPD2
//
// Fully rewriten for this project

#pragma once

#include <Adafruit_GFX.h>

#include "hardware.h"

class Display : public Adafruit_GFX {
  static constexpr uint8_t WIDTH = 200;
  static constexpr uint8_t HEIGHT = WIDTH;
  static constexpr uint8_t WB_BITMAP = (WIDTH + 7) / 8;

  static constexpr bool kReduceBoosterTime = true; // Saves ~200ms + Reduce power usage
  static constexpr bool kFastUpdateTemp = true; // Saves 5ms + FixedSpeedier LUT (300ms update)
  
  // The display will remember the config and RAM between runs
  // we can remember them and avoid extra SPI calls
  struct DisplayState {
    bool initialized : 1 {false};
    bool backBufferValid : 1 {false};
    bool darkBorder : 1 {false};
    bool partial : 1 {false};
  };
  static DisplayState state;

public:
  uint8_t buffer[WB_BITMAP * HEIGHT];

  SPISettings _spi_settings{20000000, MSBFIRST, SPI_MODE0};

  Display();

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
  // Heavily optimize this
  void drawPixel(int16_t x, int16_t y, uint16_t color)
  {
    // if ((x < 0) || (x >= width()) || (y < 0) || (y >= height())) return;
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
    auto& ptr = buffer[x / 8 + y * WB_BITMAP];
    if (color)
      ptr |= 1 << (7 - x % 8);
    else
      ptr &= ~(1 << (7 - x % 8));
  }
  void fillScreen(uint16_t color) // 0x0 black, >0x0 white, to buffer
  {
    memset(buffer, (color) ? 0xFF : 0x00, sizeof(buffer));
  }
  // Optimized for our case from Canvas1
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  void drawFastRawVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  void drawFastRawHLine(int16_t x, int16_t y, int16_t w, uint16_t color);

  void setDarkBorder(bool darkBorder);

  void refresh(bool partial = true);
  void hibernate();
  void waitWhileBusy();
  void writeRegion(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
  void writeAll(bool backBuffer = false);
  void writeAllAndRefresh(bool partial = true);

private:
  // Called internally in middle of transfer
  void _setRamArea(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
  void _setRefreshMode(bool partial);

  // SPI transfer methods
  void _startTransfer();
  void _transfer(uint8_t value);
  void _transfer(const uint8_t* value, size_t size);
  void _transferCommand(uint8_t c);
  void _endTransfer();
};