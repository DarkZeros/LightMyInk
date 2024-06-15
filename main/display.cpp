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
// Link1: https://github.com/sqfmi/Watchy

#include "display.h"
#include "hardware.h"
#include "driver/gpio.h"

bool Display::displayFullInit = true;

void Display::busyCallback(const void *) {
  gpio_wakeup_enable((gpio_num_t)HW::DisplayPin::Busy, GPIO_INTR_LOW_LEVEL);

  // Turn OFF the FLASH during this long sleep?
  // esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_OFF);
  // ESP_LOGE("lisghtSleep", "%ld", micros());
  
  esp_sleep_enable_gpio_wakeup();
  esp_light_sleep_start();
}

Display::Display() :
  GxEPD2_EPD(HW::DisplayPin::Cs, HW::DisplayPin::Dc, HW::DisplayPin::Res, HW::DisplayPin::Busy, HIGH, 10000000, WIDTH, HEIGHT, panel, hasColor, hasPartialUpdate, hasFastPartialUpdate)
{
  // ESP_LOGE("", "boot D %lu", micros());
  // Setup callback and SPI by default
  selectSPI(SPI, SPISettings(20000000, MSBFIRST, SPI_MODE0));
  setBusyCallback(busyCallback);
}

void Display::initDisplay() {
  // default initialization
  init(0, displayFullInit, 1, true);
  _InitDisplay();
  _power_is_on = true; // This will avoid any power on calls, not needed because on refreh will power on the boosters
}

void Display::setDarkBorder(bool dark) {
  if (_hibernating) return;
  darkBorder = dark;
  _startTransfer();
  _transferCommand(0x3C); // BorderWavefrom
  _transfer(dark ? 0x02 : 0x05);
  _endTransfer();
}

void Display::clearScreen(uint8_t value)
{
  writeScreenBuffer(value);
  refresh(true);
  writeScreenBufferAgain(value);
}

void Display::writeScreenBuffer(uint8_t value)
{
  if (!_using_partial_mode) _Init_Part();
  if (_initial_write) _writeScreenBuffer(0x26, value); // set previous
  _writeScreenBuffer(0x24, value); // set current
  _initial_write = false; // initial full screen buffer clean done
}

void Display::writeScreenBufferAgain(uint8_t value)
{
  if (!_using_partial_mode) _Init_Part();
  _writeScreenBuffer(0x24, value); // set current
}

void Display::_writeScreenBuffer(uint8_t command, uint8_t value)
{
  _startTransfer();
  _transferCommand(command);
  for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(HEIGHT) / 8; i++)
  {
    _transfer(value);
  }
  _endTransfer();
}

void Display::writeImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  _writeImage(0x24, bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void Display::writeImageForFullRefresh(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  _writeImage(0x26, bitmap, x, y, w, h, invert, mirror_y, pgm);
  _writeImage(0x24, bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void Display::writeImageAgain(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  _writeImage(0x24, bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void Display::_writeImage(uint8_t command, const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (_initial_write) writeScreenBuffer(); // initial full screen buffer clean
#if defined(ESP8266) || defined(ESP32)
  yield(); // avoid wdt
#endif
  int16_t wb = (w + 7) / 8; // width bytes, bitmaps are padded
  x -= x % 8; // byte boundary
  w = wb * 8; // byte boundary
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;
  if ((w1 <= 0) || (h1 <= 0)) return;
  if (!_using_partial_mode) _Init_Part();
  _setPartialRamArea(x1, y1, w1, h1);
  _startTransfer();
  _transferCommand(command);
  if (h1 == HEIGHT && w1 == WIDTH && dx == 0 && dy == 0 && !invert) { // Optimization for common case
    // ESP_LOGE("displ", "write! dx%d dy%d wb%d", dx, dy, wb);
    _pSPIx->writeBytes(bitmap, HEIGHT*WIDTH/8);
  } else {
    for (int16_t i = 0; i < h1; i++)
    {
      for (int16_t j = 0; j < w1 / 8; j++)
      {
        uint8_t data;
        // use wb, h of bitmap for index!
        int16_t idx = mirror_y ? j + dx / 8 + ((h - 1 - (i + dy))) * wb : j + dx / 8 + (i + dy) * wb;
        if (pgm)
        {
  #if defined(__AVR) || defined(ESP8266) || defined(ESP32)
          data = pgm_read_byte(&bitmap[idx]);
  #else
          data = bitmap[idx];
  #endif
        }
        else
        {
          data = bitmap[idx];
        }
        if (invert) data = ~data;
        _transfer(data);
      }
    }
  }
  _endTransfer();
#if defined(ESP8266) || defined(ESP32)
  yield(); // avoid wdt
#endif
}

void Display::writeImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  _writeImagePart(0x24, bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void Display::writeImagePartAgain(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  _writeImagePart(0x24, bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void Display::_writeImagePart(uint8_t command, const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                     int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (_initial_write) writeScreenBuffer(); // initial full screen buffer clean
#if defined(ESP8266) || defined(ESP32)
  yield(); // avoid wdt
#endif
  if ((w_bitmap < 0) || (h_bitmap < 0) || (w < 0) || (h < 0)) return;
  if ((x_part < 0) || (x_part >= w_bitmap)) return;
  if ((y_part < 0) || (y_part >= h_bitmap)) return;
  int16_t wb_bitmap = (w_bitmap + 7) / 8; // width bytes, bitmaps are padded
  x_part -= x_part % 8; // byte boundary
  w = w_bitmap - x_part < w ? w_bitmap - x_part : w; // limit
  h = h_bitmap - y_part < h ? h_bitmap - y_part : h; // limit
  x -= x % 8; // byte boundary
  w = 8 * ((w + 7) / 8); // byte boundary, bitmaps are padded
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;
  if ((w1 <= 0) || (h1 <= 0)) return;
  if (!_using_partial_mode) _Init_Part();
  _setPartialRamArea(x1, y1, w1, h1);
  _startTransfer();
  _transferCommand(command);
  for (int16_t i = 0; i < h1; i++)
  {
    for (int16_t j = 0; j < w1 / 8; j++)
    {
      uint8_t data;
      // use wb_bitmap, h_bitmap of bitmap for index!
      int16_t idx = mirror_y ? x_part / 8 + j + dx / 8 + ((h_bitmap - 1 - (y_part + i + dy))) * wb_bitmap : x_part / 8 + j + dx / 8 + (y_part + i + dy) * wb_bitmap;
      if (pgm)
      {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
        data = pgm_read_byte(&bitmap[idx]);
#else
        data = bitmap[idx];
#endif
      }
      else
      {
        data = bitmap[idx];
      }
      if (invert) data = ~data;
      _transfer(data);
    }
  }
  _endTransfer();
#if defined(ESP8266) || defined(ESP32)
  yield(); // avoid wdt
#endif
}

void Display::writeImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    writeImage(black, x, y, w, h, invert, mirror_y, pgm);
  }
}

void Display::writeImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    writeImagePart(black, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  }
}

void Display::writeNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (data1)
  {
    writeImage(data1, x, y, w, h, invert, mirror_y, pgm);
  }
}

void Display::drawImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
  writeImageAgain(bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void Display::drawImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                   int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
  writeImagePartAgain(bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void Display::drawImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    drawImage(black, x, y, w, h, invert, mirror_y, pgm);
  }
}

void Display::drawImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                   int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    drawImagePart(black, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  }
}

void Display::drawNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (data1)
  {
    drawImage(data1, x, y, w, h, invert, mirror_y, pgm);
  }
}

void Display::refresh(bool partial_update_mode)
{
  if (partial_update_mode) refresh(0, 0, WIDTH, HEIGHT);
  else
  {
    if (_using_partial_mode) _Init_Full();
    _Update_Full();
    _initial_refresh = false; // initial full update done
  }
}

void Display::refresh(int16_t x, int16_t y, int16_t w, int16_t h)
{
  if (_initial_refresh) return refresh(false); // initial update needs be full update
  // intersection with screen
  int16_t w1 = x < 0 ? w + x : w; // reduce
  int16_t h1 = y < 0 ? h + y : h; // reduce
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  w1 = x1 + w1 < int16_t(WIDTH) ? w1 : int16_t(WIDTH) - x1; // limit
  h1 = y1 + h1 < int16_t(HEIGHT) ? h1 : int16_t(HEIGHT) - y1; // limit
  if ((w1 <= 0) || (h1 <= 0)) return; 
  // make x1, w1 multiple of 8
  w1 += x1 % 8;
  if (w1 % 8 > 0) w1 += 8 - w1 % 8;
  x1 -= x1 % 8;
  if (!_using_partial_mode) _Init_Part();
  _setPartialRamArea(x1, y1, w1, h1);
  _Update_Part();
}

void Display::powerOff()
{
  _PowerOff();
}

void Display::hibernate()
{
  //_PowerOff(); // Not needed before entering deep sleep
  if (_rst >= 0)
  {
    _startTransfer();
    _transferCommand(0x10); // deep sleep mode
    //_transfer(0x1);         // enter deep sleep
    _transfer(0x11);         // enter deep sleep, no RAM
    _endTransfer();
    _hibernating = true;
  }
}

void Display::_setPartialRamArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  _startTransfer();
  _transferCommand(0x11); // set ram entry mode
  _transfer(0x03);    // x increase, y increase : normal mode
  _transferCommand(0x44);
  _transfer(x / 8);
  _transfer((x + w - 1) / 8);
  _transferCommand(0x45);
  _transfer(y % 256);
  _transfer(y / 256);
  _transfer((y + h - 1) % 256);
  _transfer((y + h - 1) / 256);
  _transferCommand(0x4e);
  _transfer(x / 8);
  _transferCommand(0x4f);
  _transfer(y % 256);
  _transfer(y / 256);
  _endTransfer();
}

void Display::_PowerOn()
{
  if (_power_is_on)
    return;
  _startTransfer();
  _transferCommand(0x22);
  _transfer(0xf8);
  _transferCommand(0x20);
  _endTransfer();
  _waitWhileBusy("_PowerOn", power_on_time);
  _power_is_on = true;
}

void Display::_PowerOff()
{
  if (!_power_is_on)
    return;
  _startTransfer();
  _transferCommand(0x22);
  _transfer(0x83);
  _transferCommand(0x20);
  _endTransfer();
  _waitWhileBusy("_PowerOff", power_off_time);
  _power_is_on = false;
  _using_partial_mode = false;
}

void Display::_InitDisplay()
{
  if (_hibernating) _reset();

  // No need to soft reset, the Display goes to same state after hard reset
  // _writeCommand(0x12); // soft reset
  // _waitWhileBusy("_SoftReset", 10); // 10ms max according to specs*/

  _startTransfer();
  _transferCommand(0x01); // Driver output control
  _transfer(0xC7);
  _transfer(0x00);
  _transfer(0x00);

  if (kReduceBoosterTime) {
    // SSD1675B controller datasheet
    _transferCommand(0x0C); // BOOSTER_SOFT_START_CONTROL
    // Set the driving strength of GDR for all phases to maximun 0b111 -> 0xF
    // Set the minimum off time of GDR to minimum 0x4 (values below sould be same)
    _transfer(0xF4); // Phase1 Default value 0x8B
    _transfer(0xF4); // Phase2 Default value 0x9C
    _transfer(0xF4); // Phase3 Default value 0x96
    _transfer(0x00); // Duration of phases, Default 0xF = 0b00 11 11 (40ms Phase 1/2, 10ms Phase 3)
  }

  _transferCommand(0x18); // Read built-in temperature sensor
  _transfer(0x80);
  _endTransfer();

  setDarkBorder(darkBorder);

  _setPartialRamArea(0, 0, WIDTH, HEIGHT);
}

void Display::_reset()
{
  // Call default method if not configured the same way
  if (_rst < 0 || !_pulldown_rst_mode) {
    GxEPD2_EPD::_reset();
    return;
  }
  gpio_set_level((gpio_num_t)_rst, LOW);
  pinMode(_rst, OUTPUT);
  delay(_reset_duration); // TODO: Use a timer light sleep

  // Maybe is not worth? Docs say 350us to sleep and 500us wake up
  // esp_sleep_enable_timer_wakeup( 1 * 1000);
  // esp_light_sleep_start();

  pinMode(_rst, INPUT_PULLUP);
  // Tested calling _powerOn() inmediately, and works ok, no need to sleep
  // delay(_reset_duration > 10 ? _reset_duration : 0);
  _hibernating = false;
}

void Display::_Init_Full()
{
  _InitDisplay();
  _PowerOn();
  _using_partial_mode = false;
}

void Display::_Init_Part()
{
  _InitDisplay();
  _PowerOn();
  _using_partial_mode = true;
}

void Display::_Update_Full()
{
  _startTransfer();
  _transferCommand(0x22);
  _transfer(0xf4);
  _transferCommand(0x20);
  _endTransfer();
  _waitWhileBusy("_Update_Full", full_refresh_time);
  displayFullInit = false;
}

void Display::_Update_Part()
{
  //_transferLUT();

  _startTransfer();

  if (kFastUpdateTemp) {
    // Write 50ºC fixed temp (fast update)
    _transferCommand(0x1A);
    _transfer(0x32);
    _transfer(0x00);
  }

  _transferCommand(0x22);

  if (kFastUpdateTemp) {
    _transfer(0b11011100); // part update + LUT + Leave power on (we will hibernate straight away)
  } else {
    _transfer(0b11111100); // part update + Load Temp + LUT + Leave power on
  }

  // Notes:
  // _transfer(0b11011100); // part update + LUT + Leave power on
  // _transfer(0b11011111); // part update + LUT + Leave power off
  // _transfer(0b11111100); // part update + Load Temp + LUT + Leave power on
  // _transfer(0b11111111); // part update + Load Temp + LUT + Leave power off
  //1xxxxxx1 // Enable Disable clock
  //x1xxxx1x // Enable disable analog
  //xx1xxxxx // Load temp
  //xxx1xxxx // Load LUT
  //xxxx1xxx // Display mode 2
  //xxxxx1xx // Display! 

  _transferCommand(0x20);
  _endTransfer();
  _waitWhileBusy("_Update_Part", partial_refresh_time);
}

void Display::_transferLUT()
{
  if (false) {
    return;
  }

  _startTransfer();

  /*_transferCommand(0x33); // Read LUT
  unsigned char lut_partial_update_real[] =
  {
      0x10, 0x18, 0x18, 0x08, 0x18, 0x18, 0x08, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 
      0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x14, 0x44, 0x12, 0x00, 
      0x00, 0x00, 0x00, 
      0x00, 0x00
  };
  SPI.transferBytes(NULL, lut_partial_update_real, 30);
  for (int i=0; i<30; i++) {
    ESP_LOGE("LUT", "%02x", lut_partial_update_real[i]);
  }*/
  _transferCommand(0x32); // Write LUT
  /*for (int i=0; i<10; i++) { // Black LUT
    _transfer(0x00);
  }
  for (int i=0; i<10; i++) { // White LUT
    _transfer(0xFF);
  }
  for (int i=0; i<10*3; i++) { // Colour LUT
    _transfer(0x00);
  }
  for (int i=0; i<10; i++) { // Phases length + Repeats
    _transfer(0x00); // A
    _transfer(0x00); // B
    _transfer(0x00); // C
    _transfer(0x00); // D
    _transfer(0x00); // RP
  }*/
  const unsigned char lut_full_update[] =
  {
      0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22, 0x66, 0x69,
      0x69, 0x59, 0x58, 0x99, 0x99, 0x88, 0x00, 0x00, 0x00, 0x00,
      //0xF8, 0xB4, 0x13, 0x51, 0x35, 0x51, 0x51, 0x19, 0x01, 0x00
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  const unsigned char VS00 = 0;
  const unsigned char VS01 = 0b01010101;
  const unsigned char VS10 = 0b10101010;
  const unsigned char VS11 = 0b11111111;
  #define REPEAT12(x) x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x,

  const unsigned char lut_partial_update[] =
  {
    // 12xVS A-D LUT 0
    REPEAT12(VS11)
    // 12xVS A-D LUT 1
    REPEAT12(VS01)
    // 12xVS A-D LUT 2
    REPEAT12(VS10)
    // 12xVS A-D LUT 3
    REPEAT12(VS00)
    // 12xVS A-D LUT 4 ?
    REPEAT12(VS00) 
    // 12x TPA+TPB + SRAB + TPC+TPD + SRCD + RP
    /*1,1, 0, 1,1, 0, 0,
    0,0, 0, 0,0, 0, 0,
    0,0, 0, 0,0, 0, 0,
    0,0, 0, 0,0, 0, 0,
    0,0, 0, 0,0, 0, 0,
    0,0, 0, 0,0, 0, 0,
    0,0, 0, 0,0, 0, 0,
    0,0, 0, 0,0, 0, 0,
    0,0, 0, 0,0, 0, 0,
    0,0, 0, 0,0, 0, 0,
    0,0, 0, 0,0, 0, 0,
    0,0, 0, 0,0, 0, 0,*/
    // 12x FR // Frame Rate
    /*0b00010111, 0b01110111, 0b01110111,
    0b01110111, 0b01110111, 0b01110111,
    // 12x XON // Gate ON
    0b00000000, 
    0b00000000,
    0b00000000,*/
  };
  for (int i=0; i<sizeof(lut_partial_update); i++)
    _transfer(lut_partial_update[i]);
  _endTransfer();
}

void Display::_transferCommand(uint8_t value)
{
  if (_dc >= 0) gpio_set_level((gpio_num_t)_dc, LOW);
  SPI.transfer(value);
  if (_dc >= 0) gpio_set_level((gpio_num_t)_dc, HIGH);
}
