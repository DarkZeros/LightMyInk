
#include "core.h"

Core lightMyInk;

extern "C" {
void app_main(void) {
    lightMyInk.boot();
}
}