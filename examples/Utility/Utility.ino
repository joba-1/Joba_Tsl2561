/*
Simple tests for Tsl2561Util namespace.

Copyright: Joachim Banzhaf, 2018

This file is part of the Joba_Tsl2561 Library.

   Joba_Tsl2561 is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Joba_Tsl2561 is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Joba_Tsl2561.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <Tsl2561Util.h>

Tsl2561::address_t addr[] = { Tsl2561::ADDR_GND, Tsl2561::ADDR_FLOAT, Tsl2561::ADDR_VDD };

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Serial.println("\nStarting Tsl2561Util loop");
}

void loop() {
  bool found = false;

  for( uint8_t i = 0; i < sizeof(addr)/sizeof(addr[0]); i++ ) {
    Tsl2561 *tsl = new Tsl2561(addr[i], Wire);
    if( tsl->available() ) {
      found = true;
      Serial.println();

      uint16_t scaledFull = 0, scaledIr;
      uint32_t full, ir, milliLux;
      uint8_t id;
      bool gain;
      Tsl2561::exposure_t exposure;

      for( uint8_t g=0; g<2; g++ ) {
        gain = g;
        for( uint8_t e=0; e<3; e++ ) {
          exposure = (Tsl2561::exposure_t)e;

          tsl->on();
          tsl->setSensitivity(gain, exposure);
          switch( exposure ) {
            case Tsl2561::EXP_402: delay(410); break;
            case Tsl2561::EXP_101: delay(110); break;
            case Tsl2561::EXP_14:  delay( 20); break;
          }
          tsl->id(id);
          tsl->fullLuminosity(scaledFull);
          tsl->irLuminosity(scaledIr);
          tsl->getSensitivity(gain, exposure);

          full = scaledFull;
          ir = scaledIr;

          if( Tsl2561Util::normalizedLuminosity(gain, exposure, full, ir) ) {
            if( Tsl2561Util::milliLux(full, ir, milliLux, Tsl2561::packageCS(id)) ) {
              Serial.printf("Tsl2561 addr: 0x%02x, id: 0x%02x, sfull: %5u, sir: %5u, full: %5u, ir: %5u, gain: %d, exp: %d, lux: %5u.%03u\n", addr[i], id, scaledFull, scaledIr, full, ir, gain, exposure, milliLux/1000, milliLux%1000);
            }
            else {
              Serial.printf("addr: 0x%02x, id: 0x%02x, full: %u, ir: %u, gain: %d, exp: %u\n", tsl->address(), id, full, ir, gain, exposure);
              Serial.println("Tsl2561Util::milliLux() error");
            }
          }
          else {
            Serial.printf("addr: 0x%02x, id: 0x%02x, full: %u, ir: %u, gain: %d, exp: %u\n", tsl->address(), id, full, ir, gain, exposure);
            Serial.println("Tsl2561Util::normalizedLuminosity() error");
          }

          tsl->off();
        }
      }
    }
  }
  if( !found ) {
    Serial.println("No Tsl2561 found. Check wiring.");
  }

  delay(5000);
}
