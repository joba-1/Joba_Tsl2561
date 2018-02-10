/*
 * Simple tests for Tsl2561 class.
 * No error checking is done.
 */

#include <Tsl2561.h>

Tsl2561::address_t addr[] = { Tsl2561::ADDR_GND, Tsl2561::ADDR_FLOAT, Tsl2561::ADDR_VDD };

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Serial.println("\nStarting Tsl2561 simple loop");
}

void loop() {
  bool found = false;

  for( uint8_t i = 0; i < sizeof(addr)/sizeof(addr[0]); i++ ) {
    Tsl2561 *tsl = new Tsl2561(addr[i], Wire);
    if( tsl->available() ) {
      found = true;

      tsl->on();
      tsl->setSensitivity(true, Tsl2561::EXP_14);
      delay(14);

      uint8_t id;
      uint16_t full, ir;
      tsl->id(id);
      tsl->fullLuminosity(full);
      tsl->irLuminosity(ir);
      Serial.printf("Tsl2561 at 0x%02x(id=0x%02x) luminosity is %5u (full) and %5u (ir)\n", addr[i], id, full, ir);

      tsl->off();
    }
  }

  if( !found ) {
    Serial.println("No Tsl2561 found. Check wiring.");
  }

  delay(5000);
}
