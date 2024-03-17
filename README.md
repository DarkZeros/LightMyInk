# LightMyInk project

E-Ink ESP32 powered Watch that can run solely on solar power

## How to build?

It should be quite straightforward:
Just clone everything (`git submodule update --init --recursive`)
`./setup.sh && idf.py build flash monitor`

## What is it?

The main porpose of LightMyInk project is to have a functional and basic E-Ink watch that can be powered via Solar cell.
The project requires specific HW, and is based around the "Watchy" idea. But differs in:
* Has a better TPS63900 power source of 2.6/2.9V
  * More efficient, less voltage, and dinamic adjust
  * This is enough to power ESP32/RTC/Eink and WiFi under all conditions
* Does not have accelerometer (too much power!)
* Does not have Battery charging
  * Should be power by the solar cell!
* There are pins for piezo speaker
* There are pins for LED light
* There are pins/points for capacitive touch

The project goals are:
[ ] Time accurate
    [ ] NTP sync - Wifi - AutoZone
    [ ] Calculate drift and adjust it
    [ ] Thermal Adjust the drift
    [ ] 1ppm Target (now 10ppm)
[ ] Battery Control
    [ ] Track battery & estimate
    [ ] Dinamic adjust Refresh rate
    [ ] Night time, saving ours
[ ] Different UI options
    [ ] Moon Indicator
    [ ] SunSet Sunrise
    [ ] Tides
[ ] Alarms
[ ] RTC connection - Configuration
[ ] Touch UI - Settings
    [ ] Capacitive Touch functionality
    [ ] Settings Menu & Storage to NVS
[ ] Vibrator
[ ] Piezoelectric
[ ] LED led
[ ] BT - Companion App ?

All this while preserving an ultra low power profile.
Ideally we should aim to render/update/configure in the time the display circuit takes to boot up.

## Helping the project

Just push to a non protected branch and ask for PR.
Any contribution is welcomed.
