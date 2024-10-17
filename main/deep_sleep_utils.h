
#include "soc/io_mux_reg.h"

// Deep Sleep pin controls
#define GPIO_MODE_INPUT(pin) \
  PIN_FUNC_SELECT(GPIO_PIN_REG_##pin, PIN_FUNC_GPIO); \
  PIN_INPUT_ENABLE(GPIO_PIN_REG_##pin); \
  REG_WRITE(GPIO_ENABLE_W1TC_REG, (1UL << pin));

#define GPIO_MODE_OUTPUT(pin) \
  PIN_FUNC_SELECT(GPIO_PIN_REG_##pin, PIN_FUNC_GPIO); \
  PIN_INPUT_DISABLE(GPIO_PIN_REG_##pin); \
  REG_WRITE(GPIO_ENABLE_W1TC_REG, (1UL << pin));
  