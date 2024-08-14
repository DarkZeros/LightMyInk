
#include "SPI.h"
#include "soc/reg_base.h"
#include "soc/spi_struct.h"
#include "soc/dport_reg.h"
#include "soc/io_mux_reg.h"
#include "settings.h"

#define GPIO_INPUT_ENABLE(pin) \
  PIN_FUNC_SELECT(GPIO_PIN_REG_##pin, PIN_FUNC_GPIO); \
  PIN_INPUT_ENABLE(GPIO_PIN_REG_##pin); \
  REG_WRITE(GPIO_ENABLE_W1TC_REG, BIT##pin);

#define GPIO_INPUT_DISABLE(pin) \
  PIN_FUNC_SELECT(GPIO_PIN_REG_##pin, PIN_FUNC_GPIO); \
  PIN_INPUT_DISABLE(GPIO_PIN_REG_##pin); \
  REG_WRITE(GPIO_ENABLE_W1TC_REG, (1UL << pin));

namespace uSpi {
  spi_dev_t& dev = *(reinterpret_cast<volatile spi_dev_t *>(DR_REG_SPI3_BASE));

  void RTC_IRAM_ATTR init() {
    auto clockDiv = 266241; // or even 8193 for 26MHz SPI !

    // pinMode(HW::DisplayPin::Sck, INPUT);
    GPIO_INPUT_ENABLE(18);
    // TODO: Make it using the variable HW::DisplayPin::Sck

    dev.slave.trans_done = 0;
    dev.slave.val = 0;
    dev.pin.val = 0;
    dev.user.val = 0;
    dev.user1.val = 0;
    dev.ctrl.val = 0;
    dev.ctrl1.val = 0;
    dev.ctrl2.val = 0;
    dev.clock.val = 0;

    dev.user.usr_mosi = 1;
    dev.user.usr_miso = 1;
    dev.user.doutdin = 1;

    // Mode 0
    dev.pin.ck_idle_edge = 0;
    dev.user.ck_out_edge = 0;
    dev.ctrl.wr_bit_order = 0; //MSBFIRST
    dev.ctrl.rd_bit_order = 0;
    dev.clock.val = clockDiv;

    // pinMode(HW::DisplayPin::Sck, OUTPUT);
    gpio_matrix_out(HW::DisplayPin::Sck, VSPICLK_OUT_IDX, false, false);

    // pinMode(HW::DisplayPin::Mosi, OUTPUT);
    gpio_matrix_out(HW::DisplayPin::Mosi, VSPID_IN_IDX, false, false);

    dev.user.cs_setup = 1;
    dev.user.cs_hold = 1;
    // pinMode(HW::DisplayPin::Cs, OUTPUT);
    gpio_matrix_out(HW::DisplayPin::Cs, VSPICS0_OUT_IDX, false, false);
    dev.pin.val = dev.pin.val & ~((1 << 0) & SPI_SS_MASK_ALL);

    // pinMode(HW::DisplayPin::Dc, OUTPUT);
    GPIO_INPUT_DISABLE(10);
    // TODO: Make it using the variable HW::DisplayPin::Dc
  }

  void RTC_IRAM_ATTR transfer(const void *data_in, uint32_t len) {
    size_t longs = len >> 2;
    if (len & 3) {
        longs++;
    }
    uint32_t *data = (uint32_t *)data_in;
    size_t c_len = 0, c_longs = 0;

    while (len) {
        c_len = (len > 64) ? 64 : len;
        c_longs = (longs > 16) ? 16 : longs;

        dev.mosi_dlen.usr_mosi_dbitlen = (c_len * 8) - 1;
        dev.miso_dlen.usr_miso_dbitlen = 0;
        for (size_t i = 0; i < c_longs; i++) {
            dev.data_buf[i] = data[i];
        }
        dev.cmd.usr = 1;
        while (dev.cmd.usr); // Wait till previous commands have finished

        data += c_longs;
        longs -= c_longs;
        len -= c_len;
    }
  }

  void RTC_IRAM_ATTR transfer(const uint8_t data_in) {
    transfer(&data_in, 1);
  }

  void RTC_IRAM_ATTR command(uint8_t value)
  {
    GPIO_OUTPUT_SET(HW::DisplayPin::Dc, 0);
    transfer(value);
    GPIO_OUTPUT_SET(HW::DisplayPin::Dc, 1);
  }

  void RTC_IRAM_ATTR setRamArea(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    command(0x44);  // X start & end positions (Byte)
    transfer(x / 8);
    transfer((x + w - 1) / 8);
    command(0x45); // Y start & end positions (Line)
    transfer(y);
    transfer(0);
    transfer((y + h - 1));
    //_transfer(0); // No need to write this, default is 0
    command(0x4e); // X start counter
    transfer(x / 8);
    command(0x4f); // Y start counter
    transfer(y);
    //_transfer(0); // No need to write this, default is 0
  };

  void RTC_IRAM_ATTR writeArea(const uint8_t* ptr, uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    setRamArea(x, y, w, h);
    command(0x24);
    transfer(ptr, ((uint16_t)h) * w / 8);
  }

  void RTC_IRAM_ATTR dMicroseconds(uint32_t us) {
    const auto m = esp_cpu_get_cycle_count();
    const auto ticks = esp_rom_get_cpu_ticks_per_us();
    const auto e = (m + us * ticks);
    while (esp_cpu_get_cycle_count() < e) {
      asm volatile("nop");
    }
  }

  void RTC_IRAM_ATTR refresh() {
    // Set partial mode ? It should be already set
    // command(0x22);
    // transfer(0b11010100 | 0b00001000);
    // Update
    command(0x20);
  }

  void RTC_IRAM_ATTR hibernate() {
      // Sleep
      command(0x10);
      transfer(1);
  }
};