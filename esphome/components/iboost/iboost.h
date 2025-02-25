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


*************************************************************************************************


    WARNING  : MISO is connected with both D6 and D2. The MCU cannot do digitalRead with MISO(D6)
    when SPI is active, so we digitalRead(D2) instead


      PIN connections
   CC1101         NodeMCU

    CSN           D8(CS)
    CSK(CLK)      D5()
    MISO          D6(MISO) + D2(for digitalRead) (IMPORTANT CC1101-MISO + D6 + D2 are connected)
    MOSI          D7(MOSI)
    GND           GND
    VCC           3.3V

	
******************************************************************************************************/

#ifndef ESPHOME_COMPONENTS_IBOOST_IBOOST_H
#define ESPHOME_COMPONENTS_IBOOST_IBOOST_H

#pragma once

#include "esphome.h"
#include "esphome/core/log.h"
#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/cc1101/cc1101.h"

namespace esphome {
namespace iboost {
	
#define PRINTLN(x, ...) Serial.println(x, ##__VA_ARGS__)
#define PRINT(x, ...) Serial.print(x, ##__VA_ARGS__)


static const char *const TAG = "iboost";

extern sensor::Sensor *heating_import;
extern sensor::Sensor *heating_power;
extern sensor::Sensor *heating_today;
extern sensor::Sensor *heating_yesterday;
extern sensor::Sensor *heating_last_7;
extern sensor::Sensor *heating_last_28;
extern sensor::Sensor *heating_last_gt;
extern sensor::Sensor *heating_boost_time;
extern text_sensor::TextSensor *heating_mode;
extern text_sensor::TextSensor *heating_warn;

extern long today, yesterday, last7, last28, total;

class iBoost : public PollingComponent {
 public:
 
  // Constructor
  //iBoost();
  iBoost() : PollingComponent(15000){}  // Poll every 15 seconds


  // Override setup() from PollingComponent
  void setup() override;
  void loop() override;
  void update() override;
  void boost(uint8_t boost_time);
  

    void set_heating_import(sensor::Sensor *sensor) { heating_import = sensor; }
    void set_heating_power(sensor::Sensor *sensor) { heating_power = sensor; }
    void set_heating_today(sensor::Sensor *sensor) { heating_today = sensor; }
    void set_heating_yesterday(sensor::Sensor *sensor) { heating_yesterday = sensor; }
    void set_heating_last_7(sensor::Sensor *sensor) { heating_last_7 = sensor; }
    void set_heating_last_28(sensor::Sensor *sensor) { heating_last_28 = sensor; }
    void set_heating_last_gt(sensor::Sensor *sensor) { heating_last_gt = sensor; }
    void set_heating_boost_time(sensor::Sensor *sensor) { heating_boost_time = sensor; }
    void set_heating_mode(text_sensor::TextSensor *sensor) { heating_mode = sensor; }
    void set_heating_warn(text_sensor::TextSensor *sensor) { heating_warn = sensor; }

private:
    sensor::Sensor *heating_import{nullptr};
    sensor::Sensor *heating_power{nullptr};
    sensor::Sensor *heating_today{nullptr};
    sensor::Sensor *heating_yesterday{nullptr};
    sensor::Sensor *heating_last_7{nullptr};
    sensor::Sensor *heating_last_28{nullptr};
    sensor::Sensor *heating_last_gt{nullptr};
    sensor::Sensor *heating_boost_time{nullptr};
    text_sensor::TextSensor *heating_mode{nullptr};
    text_sensor::TextSensor *heating_warn{nullptr};

  
  esphome::cc1101::CC1101 radio;
  
  float get_setup_priority() const override { return esphome::setup_priority::LATE; }



};
extern iBoost *global_iboost;  // Declare global instance of iBoost
}  // namespace iboost
}  // namespace esphome
#endif
