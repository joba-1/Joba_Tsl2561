/*
 * Autogain tests for Tsl2561Util class.
 */

#include <Tsl2561Util.h>

Tsl2561::address_t addr[] = { Tsl2561::ADDR_GND, Tsl2561::ADDR_FLOAT, Tsl2561::ADDR_VDD };
Tsl2561 *tsl = 0;
uint8_t id;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  for( uint8_t i = 0; i < sizeof(addr)/sizeof(addr[0]); i++ ) {
    tsl = new Tsl2561(addr[i], Wire);
    if( tsl->available() ) {
      break;
    }
    else {
      delete tsl;
      tsl = 0;
    }
  }
  Serial.println("\nStarting Tsl2561Util autogain loop");
  tsl->on(); // this reboots if no Tsl2561 available
  tsl->id(id);
}

void loop() {
  uint16_t scaledFull = ~0, scaledIr = ~0;
  uint32_t full = ~0, ir = ~0, milliLux = ~0;
  bool gain = false;
  Tsl2561::exposure_t exposure = Tsl2561::EXP_OFF;

  if( Tsl2561Util::autoGain(*tsl, gain, exposure, scaledFull, scaledIr)
   && Tsl2561Util::normalizedLuminosity(gain, exposure, full = scaledFull, ir = scaledIr)
   && Tsl2561Util::milliLux(full, ir, milliLux, Tsl2561::packageCS(id)) ) {
    Serial.printf("Tsl2561 addr: 0x%02x, id: 0x%02x, sfull: %5u, sir: %5u, full: %5u, ir: %5u, gain: %d, exp: %d, lux: %5u.%03u\n", tsl->address(), id, scaledFull, scaledIr, full, ir, gain, exposure, milliLux/1000, milliLux%1000);
  }
  else {
    Serial.printf("addr: 0x%02x, id: 0x%02x, full: %u, ir: %u, gain: %d, exp: %u\n", tsl->address(), id, full, ir, gain, exposure);
    Serial.println("Tsl2561Util::autoGain() error");
  }

  delay(1000);
}
