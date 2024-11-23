
#include "core.h"

Core& getInstance() {
    static Core instance; // Lazily initialized on first call
    return instance;
}

extern "C" {
void app_main(void) {
    getInstance().boot();
}
}