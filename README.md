# ESP-Home-iBoost
ESPHOME Marlec iboost integration

This project will allow you to monitor and control Marlec iBoost+ with Home Assistant.

It is based on a ESP8266 board and a CC1011 868MHz module.

PIN CONNECTIONS:

   CC1101         NodeMCU

    CSN           D8(CS)
    CSK(CLK)      D5()
    MISO          D6(MISO) + D2(for digitalRead) (IMPORTANT CC1101-MISO + D6 + D2 are connected)
    MOSI          D7(MOSI)
    GND           GND
    VCC           3.3V

WARNING  : MISO is connected with both D6 and D2. The MCU cannot do digitalRead with MISO(D6)
    when SPI is active, so we digitalRead(D2) instead
    
    
The module pinout (component side) is:
       __________________________
VCC   |                          |
GND   |                          |  
MOSI  |                          |GND
CLK   |                          |ANT
MISO  |                          |GND
GDO2  |                          |
GDO0  |                          |
CSN   |__________________________|

