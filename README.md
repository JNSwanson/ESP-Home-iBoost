# ESP-Home-iBoost
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
You can buy a better Xtal like Epson X1E0000210666 from Farnell (2471832), or change the frequency in the `iBoost.h` file.

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

To make these changes you will need to edit `iBoost.h` and change these lines:

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

When you make a change to `iBoost.h` you will need to rebuild and re-flash your ESP8266 device with the new code.

## Building the project

This project needs to be built from within [ESPHome](https://esphome.io/).  You can do this from Home Assistant if you have the relevant add-on installed (you need to be running Home Assistant supervisor for this) or from the ESPHome [command line interface](https://esphome.io/guides/getting_started_command_line.html) which can be installed as a Python module or as a Docker container.  Full details on how to install ESPHome are available on the ESP web page.

Once ESPHome is installed you need to copy all the files from this repo in to the ESPHome working directory (called "config" directory in the Docker version) and either:

Run `esphome run iBoost.yaml` from the command line.

Or

Run the Docker container which provides a web interface, point your browser at `http://127.0.0.1:6052` and locate the `iBoost` device in the list, click the menu button and choose `Install`.

If you receive an error along the lines of:
```undefined reference to _ZN6CC110113writeRegisterEhh'```
then you should copy `CC1101_*` in to `.esphome/build/cc1101/src/` and try again.

I have included a slightly modified version of CC1101_RF.  The only modification is to make all the functions public as I needed access to the low level stuff.

