// based on Demo Example from Good Display, available here: http://www.e-paper-display.com/download_detail/downloadsId=806.html
// Panel: GDEH0154D67 : http://www.e-paper-display.com/products_detail/productId=455.html
// Controller : SSD1681 : http://www.e-paper-display.com/download_detail/downloadsId=825.html
//
// Inspired on GxEPD2 by Author: Jean-Marc Zingg
// Library: https://github.com/ZinggJM/GxEPD2
//
// Fully rewriten for this project

#include "display.h"
#include "power.h"

#include <cstring>
#include "hardware.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"

#include "uspi.h"

// The display will remember the config and RAM between runs
// we can remember them and avoid expensive SPI calls
static RTC_DATA_ATTR struct DisplayState {
  bool initialized : 1 {false};
  bool fullMode : 1 {false};
  bool firstRefreshDone : 1 {false};
  bool darkBorder : 1 {false};
  bool inverted : 1 {false};
  bool postInvert : 1 {false};
  DisplayMode mode {DisplayMode::FULL};
} kState;

// The framebuffer mirrors the controller front RAM (0x24) and survives deep
// sleep, so a wake only has to push what it actually changed.
RTC_FAST_ATTR uint8_t Display::buffer[Display::kBufferSize] = {};
// One bit per framebuffer byte. Lives next to the buffer in RTC so a wake that
// draws but sleeps before refreshing does not lose track of what it dirtied.
RTC_FAST_ATTR std::bitset<Display::kChangesBits> Display::changes = {};

namespace {
  // WriteThrough only: bounding span (buffer indices) of everything written
  // since the last refresh, used to resync the back RAM afterwards.
  RTC_FAST_ATTR size_t kSpanLo {Display::kBufferSize};
  RTC_FAST_ATTR size_t kSpanHi {0};

  // Transient, per boot: is a write-through SPI transaction open, and which
  // buffer index is the controller address counter sitting at.
  bool sWtOpen {false};
  size_t sWtNext {Display::kBufferSize};
}

int RTC_IRAM_ATTR getSetDisplayMode() { return kState.mode; };

namespace {
  const SPISettings kSpiSettings{Display::kOverdriveSPI ? 26'666'666 : 20'000'000, MSBFIRST, SPI_MODE0};
}

SemaphoreHandle_t sSem = NULL;
void isr(void* ) {
  BaseType_t woken;
  gpio_intr_disable((gpio_num_t)HW::Display::Busy);
  xSemaphoreGiveFromISR(sSem, &woken);
}

// SPI related
void Display::_startTransfer()
{
  if constexpr (kArduinoSPI)
    SPI.beginTransaction(kSpiSettings);
  gpio_set_level((gpio_num_t)HW::Display::Cs, LOW);
}
void Display::_endTransfer()
{
  gpio_set_level((gpio_num_t)HW::Display::Cs, HIGH);
  if constexpr (kArduinoSPI)
    SPI.endTransaction();
}
void Display::_transfer(uint8_t value)
{
  if constexpr (kArduinoSPI) {
    SPI.write(value);
  } else {
    uSpi::write(value);
  }
}
void Display::_transfer(const uint8_t* value, size_t size)
{
  if constexpr (kArduinoSPI) {
    SPI.writeBytes(value, size);
  } else {
    uSpi::write(value, size);
  }
}
void Display::_transferCommand(uint8_t c)
{
  gpio_set_level((gpio_num_t)HW::Display::Dc, LOW);
  _transfer(c);
  gpio_set_level((gpio_num_t)HW::Display::Dc, HIGH);
}


Display::Display() : Adafruit_GFX(WIDTH, HEIGHT) {
  if constexpr (!kArduinoSPI) {
    uSpi::init(Display::kOverdriveSPI);
  }

  // Set pins
  pinMode(HW::Display::Cs, OUTPUT);
  pinMode(HW::Display::Dc, OUTPUT);
  pinMode(HW::Display::Res, OUTPUT);
  pinMode(HW::Display::Busy, INPUT);

  digitalWrite(HW::Display::Cs, HIGH);
  digitalWrite(HW::Display::Dc, HIGH);
  digitalWrite(HW::Display::Res, HIGH);
  
  // Reset HW / Exit Deep Sleep
  if (rtc_gpio_is_valid_gpio((gpio_num_t)HW::Display::Res))
    rtc_gpio_hold_dis((gpio_num_t)HW::Display::Res);
  gpio_set_level((gpio_num_t)HW::Display::Res, LOW);
  delay(1);
  gpio_set_level((gpio_num_t)HW::Display::Res, HIGH); // THE HAT BOARD NEEDS output high
  //pinMode(HW::Display::Res, INPUT_PULLUP);
  if (rtc_gpio_is_valid_gpio((gpio_num_t)HW::Display::Res))
    rtc_gpio_hold_en((gpio_num_t)HW::Display::Res);

  // Display requires ISR service for busy pin
  gpio_intr_disable((gpio_num_t)HW::Display::Busy);
  gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
  init();
}

void Display::init() {
  // This only needs to be done once
  if (kState.initialized)
    return;

  _startTransfer();
  _transferCommand(0x12); // SW reset all values to factory defaults
  _endTransfer();
  waitWhileBusy();

  _startTransfer();
  _transferCommand(0x01); // Driver output control
  _transfer(0xC7); // 0x0C7 is already the default
  _transfer(0b0);
  _transfer(0b000); // Gate scanning sequence, default 000
  // TODO: Can implement mirror Y feature with this bit 001

  if constexpr (kReduceBoosterTime) {
    // SSD1675B controller datasheet
    _transferCommand(0x0C); // BOOSTER_SOFT_START_CONTROL
    // Set the driving strength of GDR for all phases to maximun 0b111 -> 0xF
    // Set the minimum off time of GDR to minimum 0x4 (values below sould be same)
    _transfer(0xF4); // Phase1 Default value 0x8B
    _transfer(0xF4); // Phase2 Default value 0x9C
    _transfer(0xF4); // Phase3 Default value 0x96
    _transfer(0x00); // Duration of phases, Default 0xF = 0b00 11 11 (40ms Phase 1/2, 10ms Phase 3)
  }

  if constexpr (kFastUpdateTemp) {
    // Write 50ºC fixed temp (fastest update, but less quality)
    _transferCommand(0x1A);
    _transfer(0x32);
    _transfer(0x00);
  }

  // setRamAdressMode
  _transferCommand(0x11); // set ram entry mode
  _transfer(0b11);        //  0bYX adress mode (+1/-0), Default 0b11

  // Set initial refresh mode, will not change until first refresh
  _setRefreshMode(FULL);
  _endTransfer();

  kState.initialized = true;
}

void Display::_setCustomLut(const DisplayMode& mode) {
  auto& lut = [&] -> const LUT& {
    if (mode == GOOD)
      return SSD1681_LIGHTMYINK_CUSTOM_6_1;
    if (mode == QUICK)
      return SSD1681_LIGHTMYINK_CUSTOM_2_1;
    //if (mode == REPAIR)
    //  return SSD1681_LIGHTMYINK_REPAIR;
    return SSD1681_LIGHTMYINK_CUSTOM_6_1;
    // Other possible LUTS to use
    // auto& lut = SSD1681_WAVESHARE_1IN54_V2_LUT_FULL_REFRESH;
    // auto& lut = SSD1681_WAVESHARE_1IN54_V2_LUT_FAST_REFRESH;
    // auto& lut = SSD1681_WAVESHARE_1IN54_V2_LUT_FAST_REFRESH_KEEP;
  }();

  /* Always write main part of LUT register */
  _transferCommand(0x32);
  _transfer(lut.get().data(), 153);
  // End Option (EOPT)
  if (lut.eopt) {
    _transferCommand(0x3F); // set Lut option End
    _transfer(*lut.eopt);
  }
  /* GATE_DRIVING_VOLTAGE */
  if (lut.vgh) {
    _transferCommand(0x03);
    _transfer(*lut.vgh);
  }
  /* SRC_DRIVING_VOLTAGE */
  if (lut.vsh1_vsh2_vsl) {
    _transferCommand(0x04);
    _transfer(lut.vsh1_vsh2_vsl->data(), 3);
  }
  /* SET_VCOM_REG */
  if (lut.vcom) {
    _transferCommand(0x2c);
    _transfer(*lut.vcom);
  }
}

void Display::_setRamArea(const Rect& rect){
  auto& [x, y, w, h] = rect;
  _transferCommand(0x44);  // X start & end positions (Byte)
  _transfer(x >> 3);
  _transfer((x + w - 1) >> 3);
  _transferCommand(0x45); // Y start & end positions (Line)
  _transfer(y);
  _transfer(0);
  _transfer(y + h - 1);
  //_transfer(0); // No need to write this, default is 0
  _transferCommand(0x4e); // X start counter
  _transfer(x >> 3);
  _transferCommand(0x4f); // Y start counter
  _transfer(y);
  //_transfer(0); // No need to write this, default is 0
}

// Point the RAM address counter at a framebuffer index. Assumes the RAM area
// spans the full width, so the counter wraps to the next line on its own.
void Display::_setRamPos(size_t index)
{
  _transferCommand(0x4e); // X start counter
  _transfer(index % WB_BITMAP);
  _transferCommand(0x4f); // Y start counter
  _transfer(index / WB_BITMAP);
}

// ---- Framebuffer access ---------------------------------------------------

void Display::setByte(size_t index, uint8_t value)
{
  if constexpr (kTrack == Track::None) {
    buffer[index] = value;
    return;
  }

  // Only a real change is worth tracking: this is what makes a redraw of
  // identical content free, and it is why clear-then-redraw still ends up
  // cheap as long as it goes through the accessors.
  if (buffer[index] == value)
    return;
  buffer[index] = value;

  if constexpr (kTrack == Track::Deltas) {
    changes.set(index);
  } else {
    _writeThrough(index, value);
  }
}

void Display::fillBytes(size_t index, uint8_t value, size_t count)
{
  if constexpr (kTrack == Track::None) {
    memset(buffer + index, value, count);
    return;
  }
  for (size_t i = 0; i < count; i++)
    setByte(index + i, value);
}

void Display::copyBytes(size_t index, const uint8_t* src, size_t count)
{
  if constexpr (kTrack == Track::None) {
    memcpy(buffer + index, src, count);
    return;
  }
  for (size_t i = 0; i < count; i++)
    setByte(index + i, src[i]);
}

void Display::copyBytesSynced(size_t index, const uint8_t* src, size_t count)
{
  memcpy(buffer + index, src, count);
  markSynced(index, count);
}

void Display::markDirty(size_t index, size_t count)
{
  if constexpr (kTrack == Track::Deltas)
    for (size_t i = 0; i < count; i++)
      changes.set(index + i);
}

void Display::markSynced(size_t index, size_t count)
{
  if constexpr (kTrack == Track::Deltas)
    for (size_t i = 0; i < count; i++)
      changes.reset(index + i);
}

// ---- WriteThrough ---------------------------------------------------------

void Display::_writeThrough(size_t index, uint8_t value)
{
  if (!sWtOpen) {
    _startTransfer();
    _setRamArea({0, 0, WIDTH, HEIGHT});
    sWtOpen = true;
    sWtNext = kBufferSize; // Force a reposition for the first byte
  }
  if (index != sWtNext) {
    _setRamPos(index);
    _transferCommand(0x24);
  }
  _transfer(value);
  sWtNext = index + 1;

  if (index < kSpanLo) kSpanLo = index;
  if (index >= kSpanHi) kSpanHi = index + 1;
}

// Any other transfer would move the address counter under us, so the streaming
// transaction has to be closed before it runs. Also keeps CS from being held
// low across code that may want the shared SPI bus (Radio).
void Display::_flushWriteThrough()
{
  if constexpr (kTrack != Track::WriteThrough)
    return;
  if (!sWtOpen)
    return;
  _endTransfer();
  sWtOpen = false;
  sWtNext = kBufferSize;
}

// WriteThrough has no per-byte record left by the time we refresh, so the back
// RAM is resynced from the bounding span of everything written this round.
void Display::_writeDirtySpan(bool backbuffer)
{
  if (kSpanLo >= kSpanHi)
    return;
  _startTransfer();
  _setRamArea({0, 0, WIDTH, HEIGHT});
  _setRamPos(kSpanLo);
  _transferCommand(backbuffer ? 0x26 : 0x24);
  _transfer(buffer + kSpanLo, kSpanHi - kSpanLo);
  _endTransfer();
}

void Display::setRefreshMode(DisplayMode mode)
{
  if (kState.mode == mode)
    return;
  kState.mode = mode;

  // Can´t change the refresh mode until first frame is rendered
  if (!kState.firstRefreshDone)
    return;

  _flushWriteThrough();
  _startTransfer();
  _setRefreshMode(mode);
  _endTransfer();
}
DisplayMode Display::getRefreshMode() const
{
  return kState.mode;
}

void Display::_setRefreshMode(const DisplayMode& mode)
{
  constexpr auto kTurnOn = 0b11000000; // Enables oscillator & analog
  constexpr auto kLoadTemp = 0b00100000;
  constexpr auto kLoadLut = 0b00010000;
  constexpr auto kPartialMode = 0b00001000;
  constexpr auto kDisplay = 0b00000100;
  // constexpr auto kTurnOff = 0b00000011; // Disables oscillator & analog

  // Default modes FAST/FULL are loaded from the ROM
  bool romLut = mode == DisplayMode::FULL || mode == DisplayMode::FAST;

  // Build default updateCommand
  uint8_t updateCommand = kTurnOn; 
  if (romLut) {
    updateCommand |= kLoadLut;
  } else {
    _setCustomLut(mode);
  }
  if (mode != DisplayMode::FULL) {
    updateCommand |= kPartialMode;
  }
  if (!kFastUpdateTemp) {
    updateCommand |= kLoadTemp;
  }

  // If we are chaging from FULL to FAST/CUSTOM need to set
  // the display into 2 buffer mode again by triggering an update
  if ((mode == DisplayMode::FULL) ^ kState.fullMode && kState.firstRefreshDone) {
    _transferCommand(0x22);
    _transfer(updateCommand);
    _transferCommand(0x20);
    _endTransfer();
    waitWhileBusy();
    _startTransfer();
  }

  _transferCommand(0x22);
  _transfer(updateCommand | kDisplay);
  kState.fullMode = mode == DisplayMode::FULL;
}

void Display::refresh()
{
  _flushWriteThrough();

  // Push the new image into the front RAM (0x24)
  if (!kState.firstRefreshDone) {
    // Nothing known to be on the panel yet: seed both RAMs with the whole
    // framebuffer so the front/back pair starts out consistent.
    writeAll(true);
    writeAll(false);
  } else if constexpr (kTrack == Track::None) {
    writeAll(false);
  } else if constexpr (kTrack == Track::Deltas) {
    writeChanges(false);
  } // WriteThrough already streamed every changed byte as it was written

  {
    auto powerLock = Power::Lock(Power::Flag::Display);
    _startTransfer();
    _transferCommand(0x20);
    _endTransfer();

    waitWhileBusy();
  }

  // The panel now shows `buffer`. Bring the back RAM (0x26) in line with it: a
  // partial update computes its transitions from old (0x26) to new (0x24), so
  // they have to match before the next round. The first refresh already wrote
  // both.
  if (kState.firstRefreshDone) {
    if constexpr (kTrack == Track::None)
      writeAll(true);
    else if constexpr (kTrack == Track::Deltas)
      writeChanges(true);
    else
      _writeDirtySpan(true);
  }
  changes.reset();
  kSpanLo = kBufferSize;
  kSpanHi = 0;

  if (!kState.firstRefreshDone) {
    _startTransfer();
    _setRefreshMode(kState.mode);
    _endTransfer();
    kState.firstRefreshDone = true;
  }

  // After a refresh, finalize the display inversion
  if (kState.postInvert) {
    kState.postInvert = false;
    _startTransfer();
    _transferCommand(0x21); // RAM for Display Update
    _transfer(kState.inverted ? 0b10001000 : 0b0); // Set both front/backbuffer
    _endTransfer();
  }
}

void Display::waitWhileBusy() {
  sSem = xSemaphoreCreateBinary();

  static constexpr gpio_config_t busy_conf = {
    .pin_bit_mask = 1ULL << HW::Display::Busy,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_LOW_LEVEL,
  };
  gpio_config(&busy_conf);
  // Setting the GPIO config reenables the interrupt

  // Set the wakeup on busy, in case tasks sleep the chip as well
  gpio_wakeup_enable((gpio_num_t)HW::Display::Busy, GPIO_INTR_LOW_LEVEL);
  esp_sleep_enable_gpio_wakeup();
  gpio_isr_handler_add((gpio_num_t)HW::Display::Busy, isr, (void*) 0);

  if (xSemaphoreTake(sSem, 10'000 / portTICK_PERIOD_MS) != pdTRUE) {
    ESP_LOGE("displ", "semaphore expired!");
  }

  gpio_isr_handler_remove((gpio_num_t)HW::Display::Busy);
  gpio_intr_disable((gpio_num_t)HW::Display::Busy);
  gpio_wakeup_disable((gpio_num_t)HW::Display::Busy);
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO);

  vSemaphoreDelete(sSem);
}

void Display::setDarkBorder(bool dark) {
  if (kState.darkBorder == dark)
    return;
  _flushWriteThrough();
  _startTransfer();
  _transferCommand(0x3C); // BorderWavefrom
  _transfer(dark ? 0x02 : 0x05);
  _endTransfer();
  kState.darkBorder = dark;
}

void Display::setInverted(bool inverted) {
  if (kState.inverted == inverted)
    return;
  kState.inverted = inverted;
  _flushWriteThrough();
  _startTransfer();
  _transferCommand(0x21); // RAM for Display Update
  if (kState.firstRefreshDone) {
    _transfer(inverted ? 0b1000 : 0b10000000); // Only invert the frontbuffer
    kState.postInvert = true; // Queue the change for backbuffer
  } else {
    _transfer(inverted ? 0b10001000 : 0b0); // Set both front/backbuffer
  }
  _endTransfer();
}

void Display::rotate(Rect& rect) const
{
  auto& [x, y, w, h] = rect;
  switch (getRotation())
  {
    case 1:
      std::swap(x, y);
      std::swap(w, h);
      x = WIDTH - x - w;
      break;
    case 2:
      x = WIDTH - x - w;
      y = HEIGHT - y - h;
      break;
    case 3:
      std::swap(x, y);
      std::swap(w, h);
      y = HEIGHT - y - h;
      break;
  }
}

void Display::alignRect(Rect& rect) const
{
  rotate(rect);
  auto& [x, y, w, h] = rect;

  // Align x down, and w up
  w = (w + (x & 7) + 7) & ~7;
  x = x & ~7;

  // Align
  // x = x & ~7; // byte boundary
  // w = WIDTH - x < w ? WIDTH - x : w; // limit
  // h = HEIGHT - y < h ? HEIGHT - y : h; // limit

  // w = (w + 7) & ~7; // byte boundary, bitmaps are padded

  // w = x + w < WIDTH ? w : WIDTH - x; // limit
  // h = y + h < HEIGHT ? h : HEIGHT - y; // limit
}

// Manual escape hatch: pushes a rect of the framebuffer to the front RAM. It
// deliberately leaves the change tracking alone, the bytes still have to reach
// the back RAM on the next refresh().
void Display::writeAlignedRect(const Rect& rect)
{
  _flushWriteThrough();
  _startTransfer();
  _setRamArea(rect);
  _transferCommand(0x24);
  auto xst = rect.x >> 3;
  for (auto i = 0; i < rect.h; i++)
  {
    auto yoffset = (rect.y + i) * WB_BITMAP;
    _transfer(buffer + xst + yoffset, rect.w >> 3);
  }
  _endTransfer();
}

void Display::writeAlignedRectPacked(const uint8_t* ptr, const Rect& rect)
{
  _flushWriteThrough();
  _startTransfer();
  _setRamArea(rect);
  // ESP_LOGE("area","%p, %d %d %d %d, size %d", ptr, x, y, w, h, ((uint16_t)h) * w / 8);
  _transferCommand(0x24);
  _transfer(ptr, ((uint16_t)rect.h * rect.w) >> 3);
  _endTransfer();

  // The controller now holds bytes we never drew: mirror them so `buffer` keeps
  // matching the front RAM, otherwise every later delta is computed against a
  // stale image.
  const auto stride = rect.w >> 3;
  for (auto i = 0; i < rect.h; i++)
    copyBytesSynced((rect.x >> 3) + (rect.y + i) * WB_BITMAP, ptr + i * stride, stride);
}

void Display::writeRect(Rect rect)
{
  alignRect(rect);
  writeAlignedRect(rect);
}

void Display::writeAll(bool backbuffer)
{
  _flushWriteThrough();
  _startTransfer();
  _setRamArea({0, 0, WIDTH, HEIGHT});
  _transferCommand(backbuffer ? 0x26 : 0x24);
  _transfer(buffer, kBufferSize);
  _endTransfer();
}

// Push only the dirty bytes, coalesced into runs so each run costs one address
// set plus a single block transfer.
void Display::writeChanges(bool backbuffer)
{
  if constexpr (kTrack != Track::Deltas)
    return;

  const uint8_t cmd = backbuffer ? 0x26 : 0x24;
  _startTransfer();
  _setRamArea({0, 0, WIDTH, HEIGHT});
  for (size_t i = 0; i < kBufferSize; ) {
    if (!changes.test(i)) {
      i++;
      continue;
    }
    size_t end = i + 1;
    while (end < kBufferSize && changes.test(end))
      end++;

    _setRamPos(i);
    _transferCommand(cmd);
    _transfer(buffer + i, end - i);
    i = end;
  }
  _endTransfer();
}

void Display::hibernate()
{
  _flushWriteThrough();
  _startTransfer();
  _transferCommand(0x10); // change deep sleep mode
  _transfer(0b01);  // mode 1 (RAM reading allowed)
  // _transfer(0b11); // mode 2 (no RAM reading allowed) // Doesn't work... why?
  // _transfer(0b10); // mode 2 as well?
  _endTransfer();
}

Rect Display::getTextRect(const char * str, int16_t xc, int16_t yc) {
  int16_t x, y;
  uint16_t w, h;
  getTextBounds(str, xc < 0 ? cursor_x : xc, yc < 0 ? cursor_y : yc, &x, &y, &w, &h);
  return {static_cast<uint8_t>(x), static_cast<uint8_t>(y),
          static_cast<uint8_t>(w), static_cast<uint8_t>(h)};
}

void Display::drawPixel(int16_t x, int16_t y, uint16_t color)
{
  // check rotation, move pixel around if necessary
  switch (getRotation())
  {
    case 1:
      std::swap(x, y);
      x = WIDTH - x - 1;
      break;
    case 2:
      x = WIDTH - x - 1;
      y = HEIGHT - y - 1;
      break;
    case 3:
      std::swap(x, y);
      y = HEIGHT - y - 1;
      break;
  }

  const size_t index = (x >> 3) + y * WB_BITMAP;
  const uint8_t mask = 1 << (7 - (x & 7));

  if (color)
    orByte(index, mask);
  else
    andByte(index, ~mask);
}

void Display::fillScreen(uint16_t color)
{
  fillBytes(0, color ? 0xFF : 0x00, kBufferSize);
}

// GFX brackets composite draws with startWrite/endWrite, which is exactly the
// window where WriteThrough can hold a single SPI transaction open.
void Display::startWrite() {}
void Display::endWrite() { _flushWriteThrough(); }

// GFX would otherwise fill a rect column by column through drawFastVLine, one
// read-modify-write per pixel. Rotating the rect once and filling raw spans
// keeps whole bytes going through the memset path in drawFastRawHLine.
void Display::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
  if (w < 0) { x += w + 1; w = -w; }
  if (h < 0) { y += h + 1; h = -h; }
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if (x + w > width())  w = width()  - x;
  if (y + h > height()) h = height() - y;
  if (w <= 0 || h <= 0)
    return;

  // A rect stays a rect under rotation, so rotate once instead of per pixel
  Rect r{static_cast<uint8_t>(x), static_cast<uint8_t>(y),
         static_cast<uint8_t>(w), static_cast<uint8_t>(h)};
  rotate(r);

  for (uint8_t row = 0; row < r.h; row++)
    drawFastRawHLine(r.x, r.y + row, r.w, color);
}

/**************************************************************************/
/*!
   @brief  Speed optimized vertical line drawing
   @param  x      Line horizontal start point
   @param  y      Line vertical start point
   @param  h      Length of vertical line to be drawn, including first point
   @param  color  Color to fill with
*/
/**************************************************************************/
void Display::drawFastVLine(int16_t x, int16_t y, int16_t h,
                               uint16_t color) {

  if (h < 0) { // Convert negative heights to positive equivalent
    h *= -1;
    y -= h - 1;
    if (y < 0) {
      h += y;
      y = 0;
    }
  }

  // Edge rejection (no-draw if totally off canvas)
  if ((x < 0) || (x >= width()) || (y >= height()) || ((y + h - 1) < 0)) {
    return;
  }

  if (y < 0) { // Clip top
    h += y;
    y = 0;
  }
  if (y + h > height()) { // Clip bottom
    h = height() - y;
  }

  if (getRotation() == 0) {
    drawFastRawVLine(x, y, h, color);
  } else if (getRotation() == 1) {
    int16_t t = x;
    x = WIDTH - 1 - y;
    y = t;
    x -= h - 1;
    drawFastRawHLine(x, y, h, color);
  } else if (getRotation() == 2) {
    x = WIDTH - 1 - x;
    y = HEIGHT - 1 - y;

    y -= h - 1;
    drawFastRawVLine(x, y, h, color);
  } else if (getRotation() == 3) {
    int16_t t = x;
    x = y;
    y = HEIGHT - 1 - t;
    drawFastRawHLine(x, y, h, color);
  }
}

/**************************************************************************/
/*!
   @brief  Speed optimized horizontal line drawing
   @param  x      Line horizontal start point
   @param  y      Line vertical start point
   @param  w      Length of horizontal line to be drawn, including first point
   @param  color  Color to fill with
*/
/**************************************************************************/
void Display::drawFastHLine(int16_t x, int16_t y, int16_t w,
                               uint16_t color) {
  if (w < 0) { // Convert negative widths to positive equivalent
    w *= -1;
    x -= w - 1;
    if (x < 0) {
      w += x;
      x = 0;
    }
  }

  // Edge rejection (no-draw if totally off canvas)
  if ((y < 0) || (y >= height()) || (x >= width()) || ((x + w - 1) < 0)) {
    return;
  }

  if (x < 0) { // Clip left
    w += x;
    x = 0;
  }
  if (x + w >= width()) { // Clip right
    w = width() - x;
  }

  if (getRotation() == 0) {
    drawFastRawHLine(x, y, w, color);
  } else if (getRotation() == 1) {
    int16_t t = x;
    x = WIDTH - 1 - y;
    y = t;
    drawFastRawVLine(x, y, w, color);
  } else if (getRotation() == 2) {
    x = WIDTH - 1 - x;
    y = HEIGHT - 1 - y;

    x -= w - 1;
    drawFastRawHLine(x, y, w, color);
  } else if (getRotation() == 3) {
    int16_t t = x;
    x = y;
    y = HEIGHT - 1 - t;
    y -= w - 1;
    drawFastRawVLine(x, y, w, color);
  }
}

/**************************************************************************/
/*!
   @brief    Speed optimized vertical line drawing into the raw canvas buffer
   @param    x   Line horizontal start point
   @param    y   Line vertical start point
   @param    h   length of vertical line to be drawn, including first point
   @param    color   Binary (on or off) color to fill with
*/
/**************************************************************************/
void Display::drawFastRawVLine(int16_t x, int16_t y, int16_t h,
                                  uint16_t color) {
  // x & y already in raw (rotation 0) coordinates, no need to transform.
  size_t index = (x >> 3) + y * WB_BITMAP;
  const uint8_t bit_mask = (0x80 >> (x & 7));

  if (color > 0) {
    for (int16_t i = 0; i < h; i++, index += WB_BITMAP)
      orByte(index, bit_mask);
  } else {
    for (int16_t i = 0; i < h; i++, index += WB_BITMAP)
      andByte(index, ~bit_mask);
  }
}

/**************************************************************************/
/*!
   @brief    Speed optimized horizontal line drawing into the raw canvas buffer
   @param    x   Line horizontal start point
   @param    y   Line vertical start point
   @param    w   length of horizontal line to be drawn, including first point
   @param    color   Binary (on or off) color to fill with
*/
/**************************************************************************/
void Display::drawFastRawHLine(int16_t x, int16_t y, int16_t w,
                                  uint16_t color) {
  // x & y already in raw (rotation 0) coordinates, no need to transform.
  size_t index = (x >> 3) + y * WB_BITMAP;
  size_t remainingWidthBits = w;

  // check to see if first byte needs to be partially filled
  if ((x & 7) > 0) {
    // create bit mask for first byte
    uint8_t startByteBitMask = 0x00;
    for (int8_t i = (x & 7); ((i < 8) && (remainingWidthBits > 0)); i++) {
      startByteBitMask |= (0x80 >> i);
      remainingWidthBits--;
    }
    if (color > 0) {
      orByte(index, startByteBitMask);
    } else {
      andByte(index, ~startByteBitMask);
    }

    index++;
  }

  // do the next remainingWidthBits bits
  if (remainingWidthBits > 0) {
    size_t remainingWholeBytes = remainingWidthBits >> 3;
    size_t lastByteBits = remainingWidthBits & 7;
    uint8_t wholeByteColor = color > 0 ? 0xFF : 0x00;

    fillBytes(index, wholeByteColor, remainingWholeBytes);

    if (lastByteBits > 0) {
      uint8_t lastByteBitMask = 0x00;
      for (size_t i = 0; i < lastByteBits; i++) {
        lastByteBitMask |= (0x80 >> i);
      }
      index += remainingWholeBytes;

      if (color > 0) {
        orByte(index, lastByteBitMask);
      } else {
        andByte(index, ~lastByteBitMask);
      }
    }
  }
}