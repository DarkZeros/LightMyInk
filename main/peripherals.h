#pragma once

#include <span>
#include <vector>
#include <inttypes.h>

/* Helper class to handle extra peripherals, buzz, speaker, light
 */
struct Peripherals {
    static void vibrator(std::vector<int>);
    static void light(uint32_t us);
    static void speaker(std::vector<std::pair<int, int>> pattern);

    static void tetris();
};