
#pragma once

/* Helper class to handle extra power needed for WiFi operations
 */
struct Power {
    static void high() { setVoltage(true); }
    static void low() { setVoltage(false); }
    static void setVoltage(bool high);
};