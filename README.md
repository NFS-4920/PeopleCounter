# PeopleCounter
Smart people counter with ESP32-Arduino and VL53L1x sensor.
## Source
Inspired by the following project
*  https://github.com/stm32duino/X-NUCLEO-53L1A1.
## Dependencys
* STM32duino VL53L1X: https://github.com/stm32duino/VL53L1X
* STM32duino Proximity Gesture: https://github.com/stm32duino/Proximity_Gesture
## Notes
The maximum detection distance is influenced by the color of the target and
the indoor or outdoor situation due to absence or presence of external
infrared.
The detection range can be comprise between ~40cm and ~400cm. (see chapter 3 of
the VL53L1X datasheet).
