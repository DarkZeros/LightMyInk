#pragma once

namespace HW {

namespace Touch {
    enum Btn {TopLeft, TopRight, BotLeft, BotRight};
    constexpr uint8_t Pin[] = {2,4,12,14};
}

namespace DisplayPin {
    constexpr uint8_t Cs = 5;
    constexpr uint8_t Res = 9;
    constexpr uint8_t Dc = 10;
    constexpr uint8_t Busy = 19;
};

constexpr uint8_t kAdcPin = 34;
constexpr uint8_t kRtcIntPin = 32;
constexpr uint8_t kLightPin = 25;
constexpr uint8_t kBuzzPin = 26;
constexpr uint8_t kVibratorPin = 27;
constexpr uint8_t kSelectPin = 13;

/*#define UP_BTN_MASK (uint64_t(1)<<GPIO_NUM_35)
#define MENU_BTN_MASK (uint64_t(1)<<GPIO_NUM_26)
#define BACK_BTN_MASK (uint64_t(1)<<GPIO_NUM_25)
#define DOWN_BTN_MASK (uint64_t(1)<<GPIO_NUM_4)
#define ACC_INT_MASK  (uint64_t(1)<<GPIO_NUM_14)
#define BTN_PIN_MASK  MENU_BTN_MASK|BACK_BTN_MASK|UP_BTN_MASK|DOWN_BTN_MASK
*/

} // namespace HW