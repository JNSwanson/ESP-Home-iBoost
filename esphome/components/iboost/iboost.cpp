/*************************************************************************************************

 ________    ______    _______    ____  ____                                 
|_   __  | .' ____ \  |_   __ \  |_   ||   _|                                
  | |_ \_| | (___ \_|   | |__) |   | |__| |     .--.    _ .--..--.    .---.  
  |  _| _   _.____`.    |  ___/    |  __  |   / .'`\ \ [ `.-. .-. |  / /__\\ 
 _| |__/ | | \____) |  _| |_      _| |  | |_  | \__. |  | | | | | |  | \__., 
|________|  \______.' |_____|    |____||____|  '.__.'  [___||__||__]  '.__.' 
                                                                             
              _    ______                                _                   
             (_)  |_   _ \                              / |_                 
             __     | |_) |    .--.     .--.    .--.   `| |-'                
            [  |    |  __'.  / .'`\ \ / .'`\ \ ( (`\]   | |                  
             | |   _| |__) | | \__. | | \__. |  `'.'.   | |,                 
            [___] |_______/   '.__.'   '.__.'  [\__) )  \__/                 
                                                                             
                                                                             

MIT License

Copyright (c) 2023 JNSwanson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.


*************************************************************************************************/
#include "iboost.h"

#include "esphome/core/log.h"

namespace esphome {
    namespace iboost {

        // Global instance
        iBoost * global_iboost = nullptr;

        enum { // codes for the various requests and responses
            SAVED_TODAY = 0xCA,
                SAVED_YESTERDAY = 0xCB,
                SAVED_LAST_7 = 0xCC,
                SAVED_LAST_28 = 0xCD,
                SAVED_TOTAL = 0xCE
        };

        enum { // packet types
            PACKET_SENDER = 0x01,
                PACKET_BUDDY = 0x21,
                PACKET_IBOOST = 0x22
        };

        enum {
            RXSTATE_WAIT_FOR_PACKET,
            RXSTATE_PROCESS_PACKET,
            RXSTATE_PUBLISH_RESULTS,
        }
        rxState;

        long today, yesterday, last7, last28, total;

        esphome::cc1101::CC1101 radio(D8, D2);

        // used for the periodic pings see below
        uint32_t pingTimer;
        // used for LED blinking when we receive a packet
        uint32_t ledTimer;

        uint32_t rxTimer;

        //static uint8_t txStart[] = {CC1101_SIDLE, CC1101_TXFIFO, CC1101_AGCCTRL0};
        uint8_t txBuf[32];
        uint8_t request;
        bool boostRequest;
        uint8_t address[2]; // this is the address of the sender
        uint8_t addressLQI, rxLQI; // signal strength test 
        bool addressValid;

        byte packet[MAX_PACKET_LEN];
        byte pkt_size;
        short heating;
        long p1, p2;
        char pbuf[32];
        byte boostTime;
        bool waterHeating, cylinderHot, batteryLow;

        // Register component
        void register_iboost() {
            global_iboost = new iBoost();
            App.register_component(global_iboost);
        }

        void iBoost::setup() {
            // Initialize text sensors
            if (heating_mode != nullptr) {
                heating_mode -> publish_state("Initializing...");
            }
            if (heating_warn != nullptr) {
                heating_warn -> publish_state("No Warnings");
            }

            // Initialize numeric sensors
            if (heating_import) heating_import -> publish_state(0);
            if (heating_power) heating_power -> publish_state(0);
            if (heating_today) heating_today -> publish_state(0);
            if (heating_yesterday) heating_yesterday -> publish_state(0);
            if (heating_last_7) heating_last_7 -> publish_state(0);
            if (heating_last_28) heating_last_28 -> publish_state(0);
            if (heating_last_gt) heating_last_gt -> publish_state(0);
            if (heating_boost_time) heating_boost_time -> publish_state(0);

            addressLQI = 255; // set received LQI to lowest value
            addressValid = false;
            SPI.begin();
            Serial.println("SPI OK");
            ESP_LOGW(TAG, "SPI OK");
            radio.reset();
            Serial.println("RadioReset");
            radio.begin(868.300e6); // Freq=868.3Mhz. Do not forget the "e6"
            Serial.println("RadioReg");
            radio.setMaxPktSize(61);
            Serial.println("RadioReg");
            radio.writeRegister(CC1101_FREQ2, 0x21); // 868.300MHz  (868300000 <<16)/26000000
            Serial.println("RadioReg");
            radio.writeRegister(CC1101_FREQ1, 0x65);
            radio.writeRegister(CC1101_FREQ0, 0x6a);
            radio.writeRegister(CC1101_FSCTRL1, 0x08); // fif=203.125kHz
            radio.writeRegister(CC1101_FSCTRL0, 0x00); // No offset
            radio.writeRegister(CC1101_MDMCFG4, 0x5B); // CHANBW_E = 1 CHANBW_M=1 BWchannel =325kHz   DRATE_E=11
            radio.writeRegister(CC1101_MDMCFG3, 0xF8); // DRATE_M=248 RDATA=99.975kBaud
            radio.writeRegister(CC1101_MDMCFG2, 0x03); // Disable digital DC blocking filter before demodulator enabled. MOD_FORMAT=000 (2-FSK) Manchester Coding disabled Combined sync-word qualifier mode = 30/32 sync word bits detected
            radio.writeRegister(CC1101_MDMCFG1, 0x22); // Forward error correction disabled 4 preamble bytes transmitted CHANSPC_E=2
            radio.writeRegister(CC1101_MDMCFG0, 0xF8); // CHANSPC_M=248 200kHz channel spacing
            radio.writeRegister(CC1101_CHANNR, 0x00); // The 8-bit unsigned channel number, which is multiplied by the channel spacing setting and added to the base frequency.
            radio.writeRegister(CC1101_DEVIATN, 0x47); // DEVIATION_E=4 DEVIATION_M=7 ±47.607 kHz deviation
            radio.writeRegister(CC1101_FREND1, 0xB6); // Adjusts RX RF device
            radio.writeRegister(CC1101_FREND0, 0x10); // Adjusts TX RF device
            radio.writeRegister(CC1101_MCSM0, 0x18); // Calibrates whngoing from IDLE to RX or TX (or FSTXON) PO_TIMEOUT 149-155uS Pin control disabled XOSC off in sleep mode
            //radio.writeRegister(CC1101_MCSM1, 0x00); // Channel clear = always Return to idle after packet reception Return to idle after transmission
            radio.writeRegister(CC1101_FOCCFG, 0x1D); // The frequency compensation loop gain to be used before a sync word is detected = 4K The frequency compensation loop gain to be used after a sync word is Detected = K/2 The saturation point for the frequency offset compensation algorithm = ±BWchannel /8
            radio.writeRegister(CC1101_BSCFG, 0x1C); // The clock recovery feedback loop integral gain to be used before a sync word is detected = KI The clock recovery feedback loop proportional gain to be used before a sync word is detected = 2KP The clock recovery feedback loop integral gain to be used after a sync word is Detected = KI/2 The clock recovery feedback loop proportional gain to be used after a sync word is detected = KP The saturation point for the data rate offset compensation algorithm = ±0 (No data rate offset compensation performed)
            radio.writeRegister(CC1101_AGCCTRL2, 0xC7); // The 3 highest DVGA gain settings can not be used. Maximum allowable LNA + LNA 2 gain relative to the maximum possible gain. Target value for the averaged amplitude from the digital channel filter = 42dB
            radio.writeRegister(CC1101_AGCCTRL1, 0x00); // LNA 2 gain is decreased to minimum before decreasing LNA gain Relative carrier sense threshold disabled Sets the absolute RSSI threshold for asserting carrier sense to MAGN_TARGET
            radio.writeRegister(CC1101_AGCCTRL0, 0xB2); // Sets the level of hysteresis on the magnitude deviation (internal AGC signal that determine gain changes) to Medium hysteresis, medium asymmetric dead zone, medium gain Sets the number of channel filter samples from a gain adjustment has been made until the AGC algorithm starts accumulating new samples to 32 samples AGC gain never frozen
            radio.writeRegister(CC1101_FSCAL3, 0xEA); // Detailed calibration
            radio.writeRegister(CC1101_FSCAL2, 0x2A); //
            radio.writeRegister(CC1101_FSCAL1, 0x00); //
            radio.writeRegister(CC1101_FSCAL0, 0x1F); //
            radio.writeRegister(CC1101_FSTEST, 0x59); // Test register
            radio.writeRegister(CC1101_TEST2, 0x81); // Values to be used from SmartRF software
            radio.writeRegister(CC1101_TEST1, 0x35); //
            radio.writeRegister(CC1101_TEST0, 0x09); //
            radio.writeRegister(CC1101_IOCFG2, 0x0B); // Active High Serial Clock
            radio.writeRegister(CC1101_IOCFG0, 0x46); // Analog temperature sensor disabled Active High Asserts when sync word has been sent / received, and de-asserts at the end of the packet
            radio.writeRegister(CC1101_PKTCTRL1, 0x04); // Sync word is always accepted Automatic flush of RX FIFO when CRC is not OK disabled Two status bytes will be appended to the payload of the packet. The status bytes contain RSSI and LQI values, as well as CRC OK. No address checkof received packages.
            radio.writeRegister(CC1101_PKTCTRL0, 0x05); // Data whitening off Normal mode, use FIFOs for RX and TX CRC calculation in TX and CRC check in RX enabled Variable packet length mode. Packet length configured by the first byte after sync word
            radio.writeRegister(CC1101_ADDR, 0x00); // Address used for packet filtration. Optional broadcast addresses are 0 (0x00) and 255 (0xFF).
            static uint8_t paTable[] = {
                0xC6,
                0x39,
                0x3A,
                0x3B,
                0x3C,
                0x3D,
                0x3E,
                0x3F
            };
            radio.writeBurstRegister(CC1101_PATABLE, paTable, sizeof(paTable));

            radio.strobe(CC1101_SIDLE); //
            radio.strobe(CC1101_SPWD); //

            Serial.println("Radio OK");
            radio.setRXstate(); // Set the current state to RX : listening for RF packets
            Serial.println("Radio RX OK");
            // LED setup. It is importand as we can use the module without serial terminal
            pinMode(LED_BUILTIN, OUTPUT);
            Serial.println("Setup Finished");
            ESP_LOGW(TAG, "Setup Finished");
        }

        void iBoost::boost(uint8_t boost_time) {
            boostTime = boost_time;
            boostRequest = true;
        }

        void iBoost::update() {
            //Serial.println("Update ****");
            if (heating_warn -> has_state()) {
                if (batteryLow)
                    heating_warn -> publish_state("Sender Battery LOW");
                else
                    heating_warn -> publish_state("");
            }
        }

        void iBoost::loop() {

            // Turn on the LED for 200ms without blocking the loop.
            // The Buildin LED on NodeMCU is ON when LOW
            digitalWrite(LED_BUILTIN, millis() - ledTimer > 200);

            if (addressValid) {
                if ((millis() - pingTimer > 10000) || boostRequest) { // ping every 10sec
                    if ((millis() - rxTimer) > 1000 && (millis() - rxTimer) < 2000) {
                        memset(txBuf, 0, sizeof(txBuf));
                        if ((request < 0xca) ||
                            (request > 0xce)
                        ) request = 0xca;

                        txBuf[1] = address[0]; //payload
                        txBuf[2] = address[1];
                        txBuf[3] = 0x21;
                        txBuf[4] = 0x8;
                        txBuf[5] = 0x92;
                        txBuf[6] = 0x7;
                        txBuf[9] = 0x24;
                        txBuf[11] = 0xa0;
                        txBuf[12] = 0xa0;
                        txBuf[13] = request; // this is the request
                        txBuf[15] = 0xa0;
                        txBuf[16] = 0xa0;
                        txBuf[17] = 0xc8;

                        if (boostRequest) {
                            txBuf[4] = 0x18; // set boost time
                            txBuf[18] = boostTime;
                            boostRequest = false;
                        }
                        radio.strobe(CC1101_SIDLE);
                        radio.writeRegister(CC1101_TXFIFO, 0x1d); // this is the packet length

                        radio.writeBurstRegister(CC1101_TXFIFO, txBuf + 1, 29); // write the data to the TX FIFO
                        radio.strobe(CC1101_STX);
                        delay(5);
                        radio.strobe(CC1101_SWOR);
                        delay(5);
                        radio.strobe(CC1101_SFRX);
                        radio.strobe(CC1101_SIDLE);
                        radio.strobe(CC1101_SRX);
                        Serial.println("Sending request");
                        request++;
                        pingTimer = millis();
                        return;
                    }
                }
            }

            switch (rxState) {
            case RXSTATE_WAIT_FOR_PACKET:

                // Receive part. if GDO0 is connected with D1 you can use it to detect incoming packets
                //if (digitalRead(D1)) {

                pkt_size = radio.getPacket(packet);
                if (pkt_size > 0 && radio.crcok()) { // We have a valid packet with some data
                    rxTimer = millis();
                    rxLQI = radio.getLQI();
                    Serial.print("Got packet from ");
                    if (packet[2] == PACKET_BUDDY) Serial.print("Buddy,");
                    else if (packet[2] == PACKET_IBOOST) Serial.print("iBoost,");
                    else if (packet[2] == PACKET_SENDER) Serial.print("Sender,");
                    else Serial.print("Unknown,");
                    Serial.print("Address=0x");
                    sprintf(pbuf, "%02x", packet[0]);
                    Serial.print(pbuf);
                    sprintf(pbuf, "%02x", packet[1]);
                    Serial.print(pbuf);
                    // Debug output
                    /*for (int i = 0; i < pkt_size; i++) {
                      sprintf(pbuf, "%02x", packet[i]);
                      Serial.print(pbuf); //packet[i], HEX);
                      Serial.print(",");
                    }*/
                    Serial.print(",len=");
                    Serial.print(pkt_size);
                    Serial.print(" Signal="); // for field tests to check the signal strength
                    Serial.print(radio.getRSSIdbm());
                    Serial.print(" LQI="); // for field tests to check the signal quality
                    Serial.println(rxLQI);
                    if ((packet[2] == PACKET_BUDDY && pkt_size == 29) // buddy request
                        ||
                        (packet[2] == PACKET_SENDER && pkt_size == 44) // sender packet
                    ) {
                        if (rxLQI < addressLQI) { // is the signal stronger than the previous/none
                            addressLQI = rxLQI;
                            address[0] = packet[0]; // save the address of the packet	0x1c7b; //
                            address[1] = packet[1];
                            addressValid = true;
                            Serial.print("Updated the address to:");
                            sprintf(pbuf, "%02x,%02x", address[0], address[1]);
                            Serial.println(pbuf);
                        }
                    }
                    ledTimer = millis();
                    rxState = RXSTATE_PROCESS_PACKET;
                }
                break;

            case RXSTATE_PROCESS_PACKET:
                Serial.println("Processing Packet");
                if (packet[2] == PACKET_IBOOST) {
                    heating = ( * (short * ) & packet[16]);
                    p1 = ( * (long * ) & packet[18]);
                    p2 = ( * (long * ) & packet[25]); // this depends on the request
                    if (packet[6])
                        waterHeating = false;
                    else
                        waterHeating = true;
                    if (packet[7])
                        cylinderHot = true;
                    else
                        cylinderHot = false;
                    boostTime = packet[5]; // boost time remaining (minutes)
                    //Serial.print("Heating=");
                    //Serial.print(heating );
                    Serial.print("P1=");
                    Serial.print(p1);
                    Serial.print(",Import=");
                    Serial.print(p1 / 360);
                    Serial.print(",P2=");
                    Serial.print(p2);
                    //Serial.print(",P3=");
                    //Serial.print((* (signed long*) &packet[29]) );
                    //Serial.print(" Power 4=");
                    //Serial.print((* (signed long*) &packet[30]) );
                    switch (packet[24]) {
                    case SAVED_TODAY:
                        today = p2;
                        break;

                    case SAVED_YESTERDAY:
                        yesterday = p2;
                        break;

                    case SAVED_LAST_7:
                        last7 = p2;
                        break;
                    case SAVED_LAST_28:
                        last28 = p2;
                        break;
                    case SAVED_TOTAL:
                        total = p2;
                        break;
                    }
                    if (cylinderHot)
                        Serial.print(",Water Tank HOT");
                    else if (boostTime > 0)
                        Serial.print(",Manual Boost ON");
                    else if (waterHeating) {
                        Serial.print(",Heating by Solar=");
                        Serial.print(heating);
                    } else
                        Serial.print(",Water Heating OFF");
                    if (batteryLow)
                        Serial.print(",Warn=Sender Battery LOW");
                    else
                        Serial.print(",Warn=");
                    Serial.print(",Today=");
                    Serial.print(today);
                    Serial.print(",Yesterday=");
                    Serial.print(yesterday);
                    Serial.print(",Last 7 Days=");
                    Serial.print(last7);
                    Serial.print(",Last 28 Days=");
                    Serial.print(last28);
                    Serial.print(",Total=");
                    Serial.print(total);
                    Serial.print(",Boost Time=");
                    Serial.println(boostTime);
                    rxState = RXSTATE_PUBLISH_RESULTS; // publish the new values
                    break;

                } else if (packet[2] == PACKET_SENDER) { // sender packet
                    if (packet[12] == 0x01) batteryLow = true;
                    else batteryLow = false;
                }
                rxState = RXSTATE_WAIT_FOR_PACKET; // no update so wait for a new packet
                break;

            case RXSTATE_PUBLISH_RESULTS:
                Serial.println("Publishing results");
                if (heating_mode -> has_state()) {
                    if (cylinderHot)
                        heating_mode -> publish_state("Water Tank HOT");
                    else if (boostTime > 0)
                        heating_mode -> publish_state("Manual Boost ON");
                    else if (waterHeating)
                        heating_mode -> publish_state("Heating by Solar");
                    else
                        heating_mode -> publish_state("Water Heating OFF");
                }
                heating_import -> publish_state(p1 / 360);
                heating_power -> publish_state(heating);
                switch (packet[24]) {
                case SAVED_TODAY:
                    heating_today -> publish_state(today);
                    break;
                case SAVED_YESTERDAY:
                    heating_yesterday -> publish_state(yesterday);
                    break;
                case SAVED_LAST_7:
                    if (last7 > 0) {
                        heating_last_7 -> publish_state(last7);
                    }
                    break;
                case SAVED_LAST_28:
                    if (last28 > 0) {
                        heating_last_28 -> publish_state(last28);
                    }
                    break;
                case SAVED_TOTAL:
                    if (total > 0) {
                        heating_last_gt -> publish_state(total);
                    }
                    break;
                }
                heating_boost_time -> publish_state(boostTime);

                rxState = RXSTATE_WAIT_FOR_PACKET;
                break;

            default:
                rxState = RXSTATE_WAIT_FOR_PACKET;
            }

        }

    } // namespace iboost
} // namespace esphome