# ESP-Home-iBoost

[!["Buy Me A Coffee"](https://www.buymeacoffee.com/assets/img/custom_images/orange_img.png)](https://www.buymeacoffee.com/jeremys)

ESPHome Marlec iboost integration

This project will allow you to monitor and control Marlec iBoost+ with Home Assistant.

It is based on a ESP8266 board and a CC1011 868MHz module.

## Pin Connections

| CC1101  | NodeMCU       |
|---------|---------------|
| CSN     | D8(CS)        |
| CSK(CLK)| D5()          |
| MISO    | D6(MISO) + D2 |
| MOSI    | D7(MOSI)      |
| GND     | GND           |
| VCC     | 3.3V          |

_IMPORTANT:_ `CC1101-MISO` + `D6` + `D2` need to be connected together.  `MISO` is connected with both `D6` and `D2`. The MCU cannot do digitalRead with `MISO(D6)` when SPI is active, so we `digitalRead(D2)` instead

## CC1101 Module Pinout

The module pinout (component side) is:
```
       ___________________
VCC   |o                  |
GND   |o                  |  
MOSI  |o                 o|GND
CLK   |o                 o|ANT
MISO  |o                 o|GND
GDO2  |o                  |
GDO0  |o                  |
CSN   |o__________________|
```

## Frequency tuning

I bought a few of these modules and most have Xtals that are a little off frequency.  This affects the quality of the received packets and can dramatically decrease the range at which you can receive packets from the iBoost.
You can buy a better Xtal like Epson X1E0000210666 from Farnell (2471832), or change the frequency in the `iBoost.cpp` file.

Below are some suggested values.  The default is 868300000.

|    Freq   | Dec     | HEX    |
|:---------:|---------|--------|
| 868425000 | 2188965 | 2166A5 |
| 868400000 | 2188902 | 216666 |
| 868375000 | 2188839 | 216627 |
| 868350000 | 2188776 | 2165E8 |
| 868325000 | 2188713 | 2165A9 |
| *868300000* | *2188650* | *21656A* |
| 868275000 | 2188587 | 21652B |
| 868250000 | 2188524 | 2164EC |
| 868225000 | 2188461 | 2164AD |
| 868200000 | 2188398 | 21646E |
| 868175000 | 2188335 | 21642F |

To make these changes you will need to clone the esphome directory structure and change the source in the `.yaml` file to point to the local directory.  Next edit `iBoost.cpp` and change these lines:

```
radio.writeRegister(CC1101_FREQ2, 0x21); // 868.300MHz  (868300000 <<16)/26000000
radio.writeRegister(CC1101_FREQ1, 0x65);
radio.writeRegister(CC1101_FREQ0, 0x6a);
```

For example, if you wanted to try 868350000 Hz, the hex is `2165E8`, so you would change those lines to read:
```
radio.writeRegister(CC1101_FREQ2, 0x21);
radio.writeRegister(CC1101_FREQ1, 0x65);
radio.writeRegister(CC1101_FREQ0, 0xe8);
```

Look at the LQI value in the debug output for an indication of received packet quality.  Lower is better.  Using the default settings a typical LQI of around 100 could be seen at approx. 5M from the iBoost.  By changing the frequency to `21 65 EB` the LQI dropped to 1 or 2.  A significant improvement in packet quality.

When looking at the debug output the bytes from the received packets are printed. The third byte represents the source of the packet:

- 01 - Clamp sensor / sender
- 21 - iBoost Buddy sending a request
- 22 - iBoost main unit metrics data

You should optimize receive quality for the iBoost main unit (0x22).

When you make a change to `iBoost.cpp` you will need to rebuild and re-flash your ESP8266 device with the new code.

## Building the project

To build the project, edit the `iBoost.yaml` and/or create a `secrets.yaml` file and  run:

`esphome run iBoost.yaml` from the command line.

The neccessary files will automatically be downloaded from this repository.


