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

I bought a few of these modules and most have Xtals that are a little off frequency.  This affects the functioning of the module.  You can buy a better Xtal like Epson X1E0000210666 from Farnell (2471832), or change the frequency in the iBoost.h file.  Below are some suggested values:

|Freq		   |Dec		|HEX  |
|--------------------------|
|868425000	|2188965	|2166A5|
868400000	2188902	216666
868375000	2188839	216627
868350000	2188776	2165E8
868325000	2188713	2165A9
868300000	2188650	21656A <- Act6ual frequency
868275000	2188587	21652B
868250000	2188524	2164EC
868225000	2188461	2164AD
868200000	2188398	21646E
868175000	2188335	21642F
                              

you will need to change these lines:
<CODE>
radio.writeRegister(CC1101_FREQ2, 0x21); // 868.300MHz  (868300000 <<16)/26000000
radio.writeRegister(CC1101_FREQ1, 0x65);
radio.writeRegister(CC1101_FREQ0, 0x6a);
</CODE>
