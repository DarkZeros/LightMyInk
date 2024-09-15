#pragma once

#include <cstdint> 

struct TouchSettings {
    uint16_t mThreshold{30};
    uint16_t mMeasCycles{1024};
    uint16_t mMeasInterval{4*4096};
    uint8_t mMap[4]{0,1,2,3};

    bool mSetup : 1 {false};
    bool mSetupMode : 1 {false};
};

class Touch {
private:
    TouchSettings& mSettings;
public:
    explicit Touch(TouchSettings& settings) : mSettings{settings} {};

    void setUp(bool onlyMenuLight);

    enum Btn {
        TopRight,
        TopLeft,
        BotLeft,
        BotRight,
        Menu=TopLeft,
        Back=BotLeft,
        Up=TopRight,
        Down=BotRight,
        Light=BotLeft};
};