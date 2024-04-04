#pragma once

#include <span>
#include <vector>

/* Helper class to handle extra peripherals, buzz, speaker, light
 */
struct Peripherals {
    static void vibrator(std::vector<int>);
    static void speaker(std::vector<std::pair<int, int>> pattern);
    static void light();

    static void tetris();
};