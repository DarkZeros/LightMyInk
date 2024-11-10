#pragma once

#include <span>
#include <vector>
#include <inttypes.h>

/* Helper class to handle extra peripherals that need active CPU control, buzz, speaker
 */
struct Peripherals {
    static void vibrator(std::vector<int>);
    static void speaker(std::vector<std::pair<int, int>> pattern);

    static void tetris();
};