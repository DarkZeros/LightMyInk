/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#pragma once

#include <array>
#include <optional>

constexpr static auto O = 0b00;
constexpr static auto B = 0b01;
constexpr static auto W = 0b10;
constexpr static auto F = 0b11;
struct LUT {
  struct Phase {
    uint8_t bb {};
    uint8_t bw {};
    uint8_t wb {};
    uint8_t ww {};
    uint8_t time {};
  };
  struct Group {
    Phase phase[4] {}; // 4 phases ABCD
    uint8_t sr[2] {}; // 2 subgroups AB, CD
    uint8_t rp {};
    uint8_t fr : 4 {}; // 0-7: 0=5Hz / 1=15Hz / 2=20Hz / 3=25Hz / 4=30Hz / 100Hz/200Hz ?
    // uint8_t xon[2]: 1; // 2 subgroups
  } group[12];
  // Extra
  std::optional<uint8_t> eopt; //  0x22 normal, 0x02 [POR], 0x07: Keep Source Level (slowly clears display)
  std::optional<uint8_t> vgh; /* POR: 0x00 == 20V, 0x03(10V)-0x17(20V) */
  std::optional<std::array<uint8_t, 3>> vsh1_vsh2_vsl;  /* POR: 0x41 (15V VSH1) 0xA8 (5V VSH2) 0x32 (-15V VSL) */
  std::optional<uint8_t> vcom; // 0x08(-0.2V) - 0x78(-3V), other values NA

  constexpr std::array<uint8_t, 153> get() const {
    std::array<uint8_t, 153> a = {};
    for (auto gi = 0; gi < std::size(group); gi++) {
      auto& g = group[gi];
      for(auto pi = 0; pi < std::size(g.phase); pi++) {
        auto& p = g.phase[pi];
        a[gi + 0*std::size(group)] |= p.bb << (6-pi*2);
        a[gi + 1*std::size(group)] |= p.bw << (6-pi*2);
        a[gi + 2*std::size(group)] |= p.wb << (6-pi*2);
        a[gi + 3*std::size(group)] |= p.ww << (6-pi*2);

        a[std::size(group)*5 + gi*7 + pi + (pi>=2 ? 1 : 0)] = p.time;
      }

      a[std::size(group)*5 + gi*7 + 2] = g.sr[0];
      a[std::size(group)*5 + gi*7 + 5] = g.sr[1];
      a[std::size(group)*5 + gi*7 + 6] = g.rp;

      a[std::size(group)*(5+7) + gi/2] |= uint8_t(g.fr) << (4*((gi+1)%2)); 
      // TODO XON
    }
    return a;
  }
};
// 0: 
constexpr auto watchLut = LUT{
  .group {
    // {
    //   .phase = {
    //   {.bb = B, .bw = W, .wb = B, .ww = W, .time = 2},
    //   },
    //   .fr = 2, // 50ms
    // },
    {
      .phase = {
        {.bb = B, .bw = W, .wb = B, .ww = W, .time = 0x1},
        {.bb = O, .bw = B, .wb = W, .ww = O, .time = 0x1},
        {.bb = O, .bw = W, .wb = B, .ww = O, .time = 0x2},
      },
      .fr = 2, // 50ms
    },
    // {
    //   .phase = {
    //   {.bb = B, .bw = W, .wb = B, .ww = W, .time = 0x1},
    //   },
    //   .fr = 7, //??ms
    // }
  },
  .eopt {0x07}, // This might degrade the display quicker but it makes it better quality
  .vgh {0x17},
  .vsh1_vsh2_vsl {{0x41, 0x0, 0x32}}, //15/0/-15
  .vcom {0x20}, // VCOM 0x20 best, 0x08 ok, 0x78 ?
};

constexpr auto SSD1681_WAVESHARE_1IN54_V2_LUT_FULL_REFRESH = LUT{
  .group {
    // Go the wrong way
    {
      .phase = {{.bb = W, .bw = B, .wb = W, .ww = B, .time = 0x0A}},
      .fr = 2,
    },
    // All Black/White with a space in between N times
    {
      .phase = {
        {.bb = B, .bw = B, .wb = B, .ww = B, .time = 0x0A},
        {.bb = O, .bw = O, .wb = O, .ww = O, .time = 0x02},
        {.bb = W, .bw = W, .wb = W, .ww = W, .time = 0x0A},
        {.bb = O, .bw = O, .wb = O, .ww = O, .time = 0x02},
      },
      .rp = 1, // Repeat 2 times
      .fr = 2,
    },
    // Go to the proper values
    {
      .phase = {{.bb = B, .bw = W, .wb = B, .ww = W, .time = 0x0A}},
      .fr = 2,
    }
  },
  .eopt {0x22}, // Normal
  .vgh {0x17}, // Max 20V
  .vsh1_vsh2_vsl {{0x41, 0x0, 0x32}}, //15/0/-15
  .vcom {0x20}, // VCOM 0x20 best
};
constexpr auto SSD1681_WAVESHARE_1IN54_V2_LUT_FAST_REFRESH = LUT{
  .group {
    {
      .phase = {{.bb = B, .bw = W, .wb = B, .ww = W, .time = 0x0A}},
      .fr = 2,
    }
  },
  .eopt {0x22}, // Normal
  .vgh {0x17}, // Max 20V
  .vsh1_vsh2_vsl {{0x41, 0x0, 0x32}}, //15/0/-15
  .vcom {0x20}, // VCOM 0x20 best
};
// NOTE: After several refreshes using SSD1681_WAVESHARE_1IN54_V2_LUT_FAST_REFRESH, you may notice the WHITE color
// goes GRAY and contrast decrease a lot. Use the LUT below to avoid that issue.
// NOTE: The LUT below will have the source output "keep previous output before power off", so the service life may be affected.
constexpr auto SSD1681_WAVESHARE_1IN54_V2_LUT_FAST_REFRESH_KEEP = []{
  auto ref = SSD1681_WAVESHARE_1IN54_V2_LUT_FAST_REFRESH;
  ref.eopt = 0x07;
  return ref;
}();
