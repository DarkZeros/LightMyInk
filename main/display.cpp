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

bool Display::displayBackBufferValid = false;

void Display::_startTransfer()
{
  SPI.beginTransaction(_spi_settings);
  gpio_set_level((gpio_num_t)HW::DisplayPin::Cs, LOW);
}

void Display::_transfer(uint8_t value)
{
  SPI.transfer(value);
}

void Display::_transfer(const uint8_t* value, size_t size)
{
  SPI.writeBytes(value, size);
}

void Display::_transferCommand(uint8_t c)
{
  gpio_set_level((gpio_num_t)HW::DisplayPin::Dc, LOW);
  SPI.transfer(c);
  gpio_set_level((gpio_num_t)HW::DisplayPin::Dc, HIGH);
}

void Display::_endTransfer()
{
  gpio_set_level((gpio_num_t)HW::DisplayPin::Cs, HIGH);
  SPI.endTransaction();
}

Display::Display() : Adafruit_GFX(WIDTH, HEIGHT) {
  // Set pins
  pinMode(HW::DisplayPin::Cs, OUTPUT);
  pinMode(HW::DisplayPin::Dc, OUTPUT);
  pinMode(HW::DisplayPin::Res, OUTPUT);
  pinMode(HW::DisplayPin::Busy, INPUT);
  digitalWrite(HW::DisplayPin::Cs, HIGH);
  digitalWrite(HW::DisplayPin::Dc, HIGH);
  digitalWrite(HW::DisplayPin::Res, HIGH);
  
  // Reset HW
  gpio_set_level((gpio_num_t)HW::DisplayPin::Res, LOW);
  pinMode(HW::DisplayPin::Res, OUTPUT);
  delay(1); // TODO: Use a timer light sleep
  // Maybe is not worth? Docs say 350us to sleep and 500us wake up
  // esp_sleep_enable_timer_wakeup( 1 * 1000);
  // esp_light_sleep_start();
  // esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
  pinMode(HW::DisplayPin::Res, INPUT_PULLUP);

  SPI.begin();

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

  // setRamAdressMode
  _transferCommand(0x11); // set ram entry mode
  _transfer(0b00000011);  //  0: -Y,-X    1: -Y,X     2: Y,-X     3: Y,X

  _endTransfer();
}

void Display::setRamArea(uint8_t x, uint8_t y, uint8_t w, uint8_t h){
  // _transferCommand(0x11); // set ram entry mode
  // _transfer(0b00000011);  //  0: -Y,-X    1: -Y,X     2: Y,-X     3: Y,X
  _transferCommand(0x44);  // X start & end positions (Byte)
  _transfer(x / 8);
  _transfer((x + w - 1) / 8);
  _transferCommand(0x45); // Y start & end positions (Line)
  _transfer(y % 256);
  _transfer(y / 256);
  _transfer((y + h - 1) % 256);
  _transfer((y + h - 1) / 256);
  _transferCommand(0x4e); // X start counter
  _transfer(x / 8);
  _transferCommand(0x4f); // Y start counter
  _transfer(y % 256);
  _transfer(y / 256);
}


void Display::refresh(bool partial)
{
  _startTransfer();

  if (kFastUpdateTemp) {
    // Write 50ºC fixed temp (fast update)
    _transferCommand(0x1A);
    _transfer(0x32);
    _transfer(0x00);
  }

  _transferCommand(0x22);

  //1xxxxxx1 // Enable Disable clock
  //x1xxxx1x // Enable disable analog
  //xx1xxxxx // Load temp
  //xxx1xxxx // Load LUT
  //xxxx1xxx // Display mode 2
  //xxxxx1xx // Display! 

  constexpr auto kTurnOnLoadLutDisplay = 0b11010100;
  constexpr auto kLoadTemp = 0b00100000;
  constexpr auto kPartialMode = 0b00001000;

  uint8_t updateCommand = kTurnOnLoadLutDisplay;
  if (!kFastUpdateTemp) {
    updateCommand |= kLoadTemp;
  }
  if (partial) {
    updateCommand |= kPartialMode;
  }
  _transfer(updateCommand);
  _transferCommand(0x20);
  _endTransfer();

  waitWhileBusy();
}

void Display::waitWhileBusy() {
  gpio_wakeup_enable((gpio_num_t)HW::DisplayPin::Busy, GPIO_INTR_LOW_LEVEL);

  // Turn OFF the FLASH during this long sleep? Not worth..
  // esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_OFF); // -0.3ms? -0.5%?
  // ESP_LOGE("lisghtSleep", "%ld", micros());
  
  esp_sleep_enable_gpio_wakeup();
  esp_light_sleep_start();
}

void Display::setDarkBorder(bool dark) {
  _startTransfer();
  _transferCommand(0x3C); // BorderWavefrom
  _transfer(dark ? 0x02 : 0x05);
  _endTransfer();
}

void Display::writeRegion(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool invert)
{
  x -= x % 8; // byte boundary
  w = WIDTH - x < w ? WIDTH - x : w; // limit
  h = HEIGHT - y < h ? HEIGHT - y : h; // limit
  w = 8 * ((w + 7) / 8); // byte boundary, bitmaps are padded
  uint8_t w1 = x + w < WIDTH ? w : WIDTH - x; // limit
  uint8_t h1 = y + h < HEIGHT ? h : HEIGHT - y; // limit
  _startTransfer();
  setRamArea(x, y, w1, h1);
  _transferCommand(0x24);
  auto xst = x / 8;
  if (!invert) {
    for (auto i = 0; i < h1; i++)
    {
      auto yoffset = (y + i) * WB_BITMAP;
      // ESP_LOGE("","pos %d, size %d", xst + yoffset, w1 / 8);
      SPI.writeBytes(buffer + xst + yoffset, w1 / 8);
    }
  } else {
    for (auto i = 0; i < h1; i++)
    {
      for (auto j = 0; j < w1 / 8; j++)
      {
        // use wb_bitmap, h_bitmap of bitmap for index!
        auto idx = xst + j + (y + i) * WB_BITMAP;
        uint8_t data = buffer[idx];
        if (invert) data = ~data;
        _transfer(data);
      }
    }
  }
  _endTransfer();
}

void Display::writeAllAndRefresh(bool partial)
{
  if (!displayBackBufferValid) {
    writeAll(true);
  }
  writeAll();
  refresh(displayBackBufferValid ? partial : false);
  // writeAll(); // Do we need to write again? or we can save it
  displayBackBufferValid = true;
}

void Display::writeAll(bool backbuffer)
{
  _startTransfer();
  setRamArea(0, 0, WIDTH, HEIGHT);
  _transferCommand(backbuffer ? 0x26 : 0x24);
  SPI.writeBytes(buffer, sizeof(buffer));
  _endTransfer();
}

void Display::hibernate()
{
  _startTransfer();
  _transferCommand(0x10); // deep sleep mode
  //_transfer(0x1);         // enter deep sleep, mode 1 (RAM reading allowed)
  _transfer(0x11);         // enter deep sleep, mode 2 (no RAM reading allowed)
  _endTransfer();
}