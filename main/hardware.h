#pragma once

#include <cstdint>

namespace HW {

namespace Touch {
    constexpr uint8_t Pin[] = {2,4,12,14};
    constexpr uint8_t Num[] = {0,2,5,6};
    constexpr uint8_t Num2Btn[] = {0, 255, 1, 255, 255, 2, 3};
}

namespace DisplayPin {
    constexpr uint8_t Cs = 5;
    constexpr uint8_t Res = 9;
    constexpr uint8_t Dc = 10;
    constexpr uint8_t Sck = 18;
    constexpr uint8_t Busy = 19;
    constexpr uint8_t Mosi = 23;
}

constexpr uint8_t kAdcPin = 34;
constexpr uint8_t kBusyIntPin = 35; // On HW v3
constexpr uint8_t kRtcIntPin = 32;
constexpr uint8_t kLightPin = 25;
constexpr uint8_t kSpeakerPin = 26;
constexpr uint8_t kVibratorPin = 27;
constexpr uint8_t kVoltageSelectPin = 13;

// constexpr uint8_t kRevision = 1; // Original
constexpr uint8_t kRevision = 2; // New 2024/Oct / Swap PowerLevels
// constexpr uint8_t kRevision = 3; // New 2024/Nov / Add Busy Interrupt

} // namespace HW