#ifndef CC1101_H
#define CC1101_H

#include <Arduino.h>
#include <SPI.h>

// CC1101 Configuration Registers
#define WRITE_BURST  0x40
#define READ_SINGLE  0x80
#define READ_BURST   0xC0
#define BYTES_IN_RXFIFO 0x7F
#define CRC_OK 0x80

// Register Addresses
#define CC1101_IOCFG2  0x00 // GDO2 Output Pin Configuration
#define CC1101_IOCFG1  0x01 // GDO1 Output Pin Configuration
#define CC1101_IOCFG0  0x02 // GDO0 Output Pin Configuration
#define CC1101_FIFOTHR 0x03 // RX FIFO and TX FIFO Thresholds
#define CC1101_SYNC1   0x04 // Sync Word, High Byte
#define CC1101_SYNC0   0x05 // Sync Word, Low Byte
#define CC1101_PKTLEN  0x06 // Packet Length
#define CC1101_PKTCTRL1 0x07 // Packet Automation Control
#define CC1101_PKTCTRL0 0x08 // Packet Automation Control
#define CC1101_ADDR    0x09 // Device Address
#define CC1101_CHANNR  0x0A // Channel Number
#define CC1101_FREQ2   0x0D // Frequency Control Word, High Byte
#define CC1101_FREQ1   0x0E // Frequency Control Word, Middle Byte
#define CC1101_FREQ0   0x0F // Frequency Control Word, Low Byte
#define CC1101_MDMCFG4 0x10 // Modem Configuration
#define CC1101_MDMCFG3 0x11 // Modem Configuration
#define CC1101_MDMCFG2 0x12 // Modem Configuration
#define CC1101_MDMCFG1 0x13 // Modem Configuration
#define CC1101_MDMCFG0 0x14 // Modem Configuration
#define CC1101_DEVIATN 0x15 // Modem Deviation Setting
#define CC1101_MCSM1   0x17 // Main Radio Control State Machine Configuration
#define CC1101_FREND0  0x22 // Front End TX Configuration
#define CC1101_FSCAL3  0x23 // Frequency Synthesizer Calibration
#define CC1101_FSCAL2  0x24 // Frequency Synthesizer Calibration
#define CC1101_FSCAL1  0x25 // Frequency Synthesizer Calibration
#define CC1101_FSCAL0  0x26 // Frequency Synthesizer Calibration

// Command Strobes
#define CC1101_SRES    0x30 // Reset Chip
#define CC1101_SRX     0x34 // Enable RX Mode
#define CC1101_STX     0x35 // Enable TX Mode
#define CC1101_SIDLE   0x36 // Exit RX/TX Mode
#define CC1101_SFRX    0x3A // Flush RX FIFO
#define CC1101_SFTX    0x3B // Flush TX FIFO
#define CC1101_SNOP    0x3D // No Operation

class CC1101 {
public:
    CC1101(uint8_t csnPin) : _csnPin(csnPin) {}
    
    void init() {
        pinMode(_csnPin, OUTPUT);
        digitalWrite(_csnPin, HIGH);
        SPI.begin();
        reset();
    }
    
    void reset() {
        digitalWrite(_csnPin, LOW);
        delayMicroseconds(10);
        digitalWrite(_csnPin, HIGH);
        delayMicroseconds(40);
        cmdStrobe(CC1101_SRES);
    }
    
    void writeReg(uint8_t addr, uint8_t value) {
        digitalWrite(_csnPin, LOW);
        SPI.transfer(addr);
        SPI.transfer(value);
        digitalWrite(_csnPin, HIGH);
    }
    
    uint8_t readReg(uint8_t addr) {
        digitalWrite(_csnPin, LOW);
        SPI.transfer(addr | READ_SINGLE);
        uint8_t value = SPI.transfer(0x00);
        digitalWrite(_csnPin, HIGH);
        return value;
    }
    
    void cmdStrobe(uint8_t cmd) {
        digitalWrite(_csnPin, LOW);
        SPI.transfer(cmd);
        digitalWrite(_csnPin, HIGH);
    }
    
    void sendData(const uint8_t* data, uint8_t length) {
        cmdStrobe(CC1101_SIDLE);
        cmdStrobe(CC1101_SFTX);
        digitalWrite(_csnPin, LOW);
        SPI.transfer(0x3F | WRITE_BURST);
        for (uint8_t i = 0; i < length; i++) {
            SPI.transfer(data[i]);
        }
        digitalWrite(_csnPin, HIGH);
        cmdStrobe(CC1101_STX);
    }

    bool receiveData(uint8_t* data, uint8_t& length) {
        uint8_t status = readReg(0x3B);
        if (status & CRC_OK) {
            length = readReg(CC1101_PKTLEN);
            digitalWrite(_csnPin, LOW);
            SPI.transfer(0x3F | READ_BURST);
            for (uint8_t i = 0; i < length; i++) {
                data[i] = SPI.transfer(0x00);
            }
            digitalWrite(_csnPin, HIGH);
            cmdStrobe(CC1101_SFRX);
            return true;
        }
        return false;
    }

private:
    uint8_t _csnPin;
};

#endif
