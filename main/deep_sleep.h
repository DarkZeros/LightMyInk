#include <inttypes.h>

// constexpr static int fixedDisplayUpdateMargin = 333'000; // This is valid for V1-V2 original displays
// constexpr static int fixedDisplayUpdateMargin = 241'000; // GoodDisplay value, with fast LUT
constexpr static int fixedDisplayUpdateMargin = 230'000; // A lower value, it will be corrected on first run

constexpr static int displayWaitReduce = 500;
constexpr static int displayWait = 100;

struct DeepSleepState {
  uint32_t updateWait {fixedDisplayUpdateMargin};
  uint8_t updateWaitReduceScale {0};

  // Display minute update variables
  uint8_t currentMinutes {0};
  uint8_t stepSize {1};
  int8_t minutes {10};
  bool redrawDec {false};
  bool displayBusy {false};
};

extern struct DeepSleepState kDSState;

extern void wake_stub_example(void);