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
#include <bitset>
#include <cstddef>
#include <cstdint>

#include "hardware.h"

#include "lut.h"

enum DisplayMode {
  FULL,
  FAST,
  GOOD,  // 6_1 // Might burn the display after days/weeks...
  QUICK, // 2_1
  // REPAIR, // Use overvoltage, and keep it long time one direction then the other
};

struct DisplaySettings {
  // Settings that can be changed
  bool mInvert {false};
  bool mDarkBorder {false};
  uint8_t mRotation {2};
  DisplayMode mMenuLut {FAST};
  DisplayMode mWatchLut {FAST};
};

extern int getSetDisplayMode();

struct Rect {
  uint8_t x, y, w, h;
  uint16_t size() const {return static_cast<uint16_t>(w)/8 * h; };
};

class Display : public Adafruit_GFX {
public:
  static constexpr bool kReduceBoosterTime = false; // Saves ~200ms + Reduce power usage, degrades quality!
  static constexpr bool kFastUpdateTemp = true; // Saves 5ms + FixedSpeedier LUT (300ms update)
  static constexpr bool kOverdriveSPI = false; // Uses a 25% faster SPI out of spec
  static constexpr bool kArduinoSPI = false; // Use the arduino SPI or the uSPI

  // How a framebuffer write reaches the controller RAM:
  //  None         : refresh() always pushes the whole framebuffer (old behaviour).
  //  Deltas       : dirty bytes are recorded in an RTC bitset and pushed by
  //                 refresh(). Costs kBufferSize/8 bytes of RTC FAST memory.
  //  WriteThrough : each changed byte is streamed to the controller the moment
  //                 it is written, so refresh() has nothing left to push to the
  //                 front RAM. Costs no tracking memory, but spends more time on
  //                 SPI while rendering and resyncs the back RAM from a bounding
  //                 span instead of the exact byte set.
  enum class Track : uint8_t { None, Deltas, WriteThrough };
  static constexpr Track kTrack = Track::Deltas;

  static constexpr uint8_t WIDTH = 200;
  static constexpr uint8_t HEIGHT = WIDTH;
  static constexpr uint8_t WB_BITMAP = (WIDTH + 7) / 8;
  static constexpr size_t kBufferSize = size_t{WB_BITMAP} * HEIGHT;

  Display();

  void init();

  void rotate(Rect& rect) const;
  void alignRect(Rect& rect) const;
  Rect getTextRect(const char * str, int16_t xc = -1, int16_t yc = -1);

  // ---- Framebuffer access -------------------------------------------------
  // `buffer` mirrors the controller front RAM (0x24) and is private on purpose:
  // every mutation has to come through one of the accessors below, they are the
  // only thing the change tracking can see. Reads are free.
  uint8_t getByte(size_t index) const { return buffer[index]; }
  const uint8_t* data(size_t index = 0) const { return buffer + index; }

  void setByte(size_t index, uint8_t value);
  void orByte(size_t index, uint8_t mask)  { setByte(index, buffer[index] |  mask); }
  void andByte(size_t index, uint8_t mask) { setByte(index, buffer[index] & mask); }
  void fillBytes(size_t index, uint8_t value, size_t count);
  void copyBytes(size_t index, const uint8_t* src, size_t count);
  // For bytes the controller has already been handed directly: updates the
  // mirror but leaves them clean, so refresh() will not push them again.
  void copyBytesSynced(size_t index, const uint8_t* src, size_t count);
  void markDirty(size_t index, size_t count = 1);
  void markSynced(size_t index, size_t count);

  // Adafruit_GFX overrides. Everything GFX draws bottoms out in one of these,
  // which is what keeps the tracking complete.
  void drawPixel(int16_t x, int16_t y, uint16_t color) override;
  void fillScreen(uint16_t color) override;
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override;
  // Optimized for our case from Canvas1
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) override;
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) override;
  void startWrite() override;
  void endWrite() override;
  void drawFastRawVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  void drawFastRawHLine(int16_t x, int16_t y, int16_t w, uint16_t color);

  void setDarkBorder(bool darkBorder);
  void setInverted(bool inverted);

  void setRefreshMode(DisplayMode mode);
  DisplayMode getRefreshMode() const;
  void refresh();
  void hibernate();
  void waitWhileBusy();
  void writeRect(Rect rect);
  void writeAlignedRect(const Rect& rect);
  void writeAlignedRectPacked(const uint8_t* ptr, const Rect& rect);
  void writeAll(bool backBuffer = false);
  void writeChanges(bool backBuffer = false);
  // refresh() decides on its own what has to be pushed now
  void writeAllAndRefresh() { refresh(); }

  // Aditional helper draw
  void drawMoon(float p, uint16_t x, uint16_t y, uint16_t radius, uint16_t on, uint16_t off);
  void fillCalcEllipse(int x, int y, uint8_t width, uint8_t height, uint16_t on);
  void fillEllipseDifference(int x, int y, uint8_t width1, uint8_t width2, uint8_t height, bool big, uint16_t color);
  void drawMoonFast(float p, int x, int y, uint8_t r, uint16_t on, uint16_t off);

private:
  // Only sized when we really track deltas, the other modes cost no RTC memory
  static constexpr size_t kChangesBits = kTrack == Track::Deltas ? kBufferSize : 1;

  static uint8_t buffer[kBufferSize];
  static std::bitset<kChangesBits> changes;

  // WriteThrough bookkeeping
  void _writeThrough(size_t index, uint8_t value);
  void _flushWriteThrough();
  void _writeDirtySpan(bool backBuffer);

  // Called internally in middle of transfer
  void _setRamArea(const Rect& rect);
  void _setRamPos(size_t index);
  void _setRefreshMode(const DisplayMode& mode);
  void _setCustomLut(const DisplayMode& mode);
  void _loadRomLut(const DisplayMode& mode);

  // SPI transfer methods
  void _startTransfer();
  void _transfer(uint8_t value);
  void _transfer(const uint8_t* value, size_t size);
  void _transferCommand(uint8_t c);
  void _endTransfer();
};
