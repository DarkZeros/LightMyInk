
#pragma once

/* This is the primary class of the project.
 * It has the entry point from deepsleep as well as all
 * the code that handles the setup/menus/misc.
 */
class Core {
    
    void boot(); // Called when ESP32 starts up
    
    void deepSleep(); // Set up device for deep sleep
};