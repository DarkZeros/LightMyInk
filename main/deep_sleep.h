#include <inttypes.h>

// constexpr static int fixedDisplayUpdateMargin = 333'000; // This is valid for V1-V2 original displays
constexpr static int fixedDisplayUpdateMargin = 241'000; // GoodDisplay value, we chose this, if it is wrong will correct

struct DeepSleepState {
  uint32_t updateWait{fixedDisplayUpdateMargin};

  uint8_t currentMinutes{0};
  uint8_t stepSize {1};
  int8_t minutes {10};

  bool displayBusy {false};
  bool redrawDec {false};
};

extern struct DeepSleepState kDSState;

extern void wake_stub_example(void);