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
#include <future>
#include <queue>

#include "hardware.h"

struct DisplaySettings {
    // Settings that can be changed
    bool mInvert : 1 {false};
    bool mDarkBorder : 1 {false};
    uint8_t mRotation : 2 {2};
};

class Display : public Adafruit_GFX {
public:
  static constexpr bool kReduceBoosterTime = true; // Saves ~200ms + Reduce power usage
  static constexpr bool kFastUpdateTemp = true; // Saves 5ms + FixedSpeedier LUT (300ms update)
  static constexpr bool kSingleSPI = false; // Assumes only display uses SPI
  static constexpr bool kCsHw = true; // Gives the SPI driver the Cs control
  static constexpr bool kOverdriveSPI = false; // Uses a 25% faster SPI out of spec

  static constexpr uint8_t WIDTH = 200;
  static constexpr uint8_t HEIGHT = WIDTH;
  static constexpr uint8_t WB_BITMAP = (WIDTH + 7) / 8;

  uint8_t buffer[WB_BITMAP * HEIGHT];

  static const SPISettings _spi_settings;

  std::queue<std::packaged_task<void(void)>> mTasks;

  Display();

  void rotate(uint8_t& x, uint8_t& y, uint8_t& w, uint8_t& h) const;
  void getAlignedRegion(uint8_t& x, uint8_t& y, uint8_t& w, uint8_t& h) const;

  // Heavily optimize this please
  void drawPixel(int16_t x, int16_t y, uint16_t color) override;
  void fillScreen(uint16_t color) override
  {
    memset(buffer, (color) ? 0xFF : 0x00, sizeof(buffer));
  }
  // Optimized for our case from Canvas1
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) override;
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) override;
  void drawFastRawVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  void drawFastRawHLine(int16_t x, int16_t y, int16_t w, uint16_t color);

  void setDarkBorder(bool darkBorder);
  void setInverted(bool inverted);

  void setRefreshMode(bool partial); // To leave the mode in a known state
  void refresh(bool partial = true);
  void hibernate();
  void waitWhileBusy();
  void writeRegion(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
  void writeRegionAligned(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
  void writeRegionAlignedPacked(const uint8_t* ptr, uint8_t x, uint8_t y, uint8_t w, uint8_t h);
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