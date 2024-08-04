#include <inttypes.h>

struct DeepSleepState {
  uint8_t currentMinutes{0};
  uint8_t stepSize {1};
  int8_t minutes {10};

  bool displayBusy {false};
  bool redrawDec {false};
};

extern struct DeepSleepState kDSState;

extern void wake_stub_example(void);