// based on Demo Example from Good Display, available here: http://www.e-paper-display.com/download_detail/downloadsId=806.html
// Panel: GDEH0154D67 : http://www.e-paper-display.com/products_detail/productId=455.html
// Controller : SSD1681 : http://www.e-paper-display.com/download_detail/downloadsId=825.html
//
// Inspired on GxEPD2 by Author: Jean-Marc Zingg
// Library: https://github.com/ZinggJM/GxEPD2
//
// Fully rewriten for this project

#include "display.h"
#include "hardware.h"
#include "driver/gpio.h"

RTC_DATA_ATTR Display::DisplayState Display::state;

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

  // This only needs to be done once
  if (!state.initialized)
  {
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

    if (kFastUpdateTemp) {
      // Write 50ºC fixed temp (fast update)
      _transferCommand(0x1A);
      _transfer(0x32);
      _transfer(0x00);
    }

    // setRamAdressMode
    _transferCommand(0x11); // set ram entry mode
    _transfer(0b00000011);  //  0: -Y,-X    1: -Y,X     2: Y,-X     3: Y,X

    _endTransfer();
    state.initialized = true;
  }
}

void Display::setRamArea(uint8_t x, uint8_t y, uint8_t w, uint8_t h){
  _transferCommand(0x44);  // X start & end positions (Byte)
  _transfer(x / 8);
  _transfer((x + w - 1) / 8);
  _transferCommand(0x45); // Y start & end positions (Line)
  _transfer(y);
  _transfer(0);
  _transfer((y + h - 1));
  //_transfer(0);
  _transferCommand(0x4e); // X start counter
  _transfer(x / 8);
  _transferCommand(0x4f); // Y start counter
  _transfer(y);
  //_transfer(0);
}


void Display::refresh(bool partial)
{
  _startTransfer();

  if (state.partial != partial)
  {
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

    _transferCommand(0x22);
    _transfer(updateCommand);
    state.partial = partial;
  }

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
  if (state.darkBorder != dark)
  {
    _startTransfer();
    _transferCommand(0x3C); // BorderWavefrom
    _transfer(dark ? 0x02 : 0x05);
    _endTransfer();
    state.darkBorder = dark;
  }
}

void Display::writeRegion(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
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
  for (auto i = 0; i < h1; i++)
  {
    auto yoffset = (y + i) * WB_BITMAP;
    SPI.writeBytes(buffer + xst + yoffset, w1 / 8);
  }
  _endTransfer();
}

void Display::writeAllAndRefresh(bool partial)
{
  if (!state.backBufferValid) {
    writeAll(true);
  }
  writeAll();
  refresh(state.backBufferValid ? partial : false);
  // writeAll(); // Do we need to write again? or we can save it
  state.backBufferValid = true;
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
  _transferCommand(0x10); // change deep sleep mode
  _transfer(0b01);  // mode 1 (RAM reading allowed)
  // _transfer(0b11); // mode 2 (no RAM reading allowed) // Doesn't work... why?
  // _transfer(0b10); // mode 2 as well?
  _endTransfer();
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
  int16_t row_bytes = ((WIDTH + 7) / 8);
  uint8_t *ptr = &buffer[(x / 8) + y * row_bytes];

  if (color > 0) {
    uint8_t bit_mask = (0x80 >> (x & 7));
    for (int16_t i = 0; i < h; i++) {
      *ptr |= bit_mask;
      ptr += row_bytes;
    }
  } else {
    uint8_t bit_mask = ~(0x80 >> (x & 7));
    for (int16_t i = 0; i < h; i++) {
      *ptr &= bit_mask;
      ptr += row_bytes;
    }
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
  int16_t rowBytes = ((WIDTH + 7) / 8);
  uint8_t *ptr = &buffer[(x / 8) + y * rowBytes];
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
      *ptr |= startByteBitMask;
    } else {
      *ptr &= ~startByteBitMask;
    }

    ptr++;
  }

  // do the next remainingWidthBits bits
  if (remainingWidthBits > 0) {
    size_t remainingWholeBytes = remainingWidthBits / 8;
    size_t lastByteBits = remainingWidthBits % 8;
    uint8_t wholeByteColor = color > 0 ? 0xFF : 0x00;

    memset(ptr, wholeByteColor, remainingWholeBytes);

    if (lastByteBits > 0) {
      uint8_t lastByteBitMask = 0x00;
      for (size_t i = 0; i < lastByteBits; i++) {
        lastByteBitMask |= (0x80 >> i);
      }
      ptr += remainingWholeBytes;

      if (color > 0) {
        *ptr |= lastByteBitMask;
      } else {
        *ptr &= ~lastByteBitMask;
      }
    }
  }
}