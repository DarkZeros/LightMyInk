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
- [X] Basic UI functionality and Widgets
  - [X] Ultra low consumption <0.5 mAh/day!
- [X] Features
  - [X] Deep Sleep Lambda
  - [X] LED light
  - [X] Vibration
  - [X] Speaker
- [X] Time accurate
  - [X] Manual calibrate drift and adjust it
  - [X] 1ppm Target (now 10ppm)
  - [ ] Thermal Adjust the drift ?
  - [ ] NTP sync - Wifi - AutoZone
- [X] Battery Power support
  - [X] Track battery & estimate
  - [X] Auto power save
  - [X] Night time, saving ours
- [X] Touch Settings
  - [X] Capacitive Touch functionality
  - [ ] Touch configuration - Sensitivity
- [ ] Watchface configuration
  - [X] Moon Indicator
  - [ ] SunSet Sunrise
  - [ ] Tides
  - [ ] Pictures, etc
- [ ] Configurable Alarms
- [ ] BT - Companion App ?

All this while preserving an ultra low power profile! :)

## Helping the project

Just push to a non protected branch and ask for PR.
Any contribution is welcomed.
