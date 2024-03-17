
#include "Watchy_7_SEG.h"
#include "settings.h"

Watchy7SEG watchy(settings);

extern "C" {
void app_main(void) {
    watchy.init();
}
}