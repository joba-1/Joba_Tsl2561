/*
Copyright: Joachim Banzhaf, 2019

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

#include "Tsl2561Util.h"

namespace Tsl2561Util {

// Tsl2561Util::normalizedLuminosity returncode false can mean:
// - saturation: full and/or ir have value ~0 (aka -1) and not shortest exposure
// - manual exposure time: full and ir are corrected only for gain
// If true, full and ir have values as if exposure was 402 and gain 16
//   or ~0 if saturated even at shortest exposure
bool normalizedLuminosity( bool gain, Tsl2561::exposure_t exposure, uint32_t &full, uint32_t &ir ) {

  uint16_t scaledFull = (uint16_t)full;
  uint16_t scaledIr = (uint16_t)ir;

  if( scaledFull ) {

    if( ! gain ) {
      full <<= 4;
      ir   <<= 4;
    }

    switch( exposure ) {
      case Tsl2561::EXP_14:
        full = (scaledFull == 5047) ? 0xffffffff : ((full + 5) * 322) / 11;
        ir   = (scaledIr   == 5047) ? 0xffffffff : ((ir   + 5) * 322) / 11;
        break;
      case Tsl2561::EXP_101:
        full = (scaledFull >= 37177/4*3) ? 0xffffffff : ((full + 40) * 322) / 81;
        ir   = (scaledIr   >= 37177/4*3) ? 0xffffffff : ((ir   + 40) * 322) / 81;
        break;
      case Tsl2561::EXP_402:
        if( scaledFull >= 65535/4*3 ) full = 0xffffffff;
        if( scaledIr   >= 65535/4*3 ) ir   = 0xffffffff;
        break;
      default:
        return false;
    }

    return exposure == Tsl2561::EXP_14 || (full != 0xffffffff && ir != 0xffffffff);
  }

  return false;
}

// Return upper saturation limit upto which chip returns accurate data
uint16_t getLimit( Tsl2561::exposure_t exposure ) {
  switch( exposure ) {
    case Tsl2561::EXP_14:  return 5047/4*3;
    case Tsl2561::EXP_101: return 37177/4*3;
    default: return 65535/4*3;
  }
}

// One measurement interval for given exposure plus some empirically tested extra millis
uint16_t getDelay( Tsl2561::exposure_t exposure ) {
  switch( exposure ) {
    case Tsl2561::EXP_14:  return Tsl2561Util::DELAY_EXP_14;  break;
    case Tsl2561::EXP_101: return Tsl2561Util::DELAY_EXP_101; break;
    default:               return Tsl2561Util::DELAY_EXP_402; break;
  }
}

// Wait for one exposure delay
void waitNext( Tsl2561::exposure_t exposure ) {
  delay(getDelay(exposure));
}

// If according to given gain, exposure and setSensMs a sample is not yet available, return true and luminosity = 0
// If a sample is available and no need to adjust sensitivity, return true, luminosity and new setSensMs
// If it is too bright, decrease retryOnSaturated. Stop retrying if retry counted down to 0
// Else (i.e. setSensMs == 0) this is the first check and we just started sampling
// setSensMs is the lower 16 bit of millis() when last measurement started, e.g. at tsl.on() or tsl.setSensitivity()
// If it is != 0, then gain and exposure are assumed to have valid values, e.g. from previous tsl.get/setSensitivity()
bool autoGainCheck( Tsl2561 &tsl, bool &gain, Tsl2561::exposure_t &exposure, uint16_t &full, uint16_t &ir, uint16_t &setSensMs, uint8_t &retryOnSaturated ) {

  static const struct {
    bool                gain;
    Tsl2561::exposure_t exposure;
  } sensitivity[] = {
    { false, Tsl2561::EXP_14, }, // min
    { false, Tsl2561::EXP_101 },
    { true,  Tsl2561::EXP_14  },
    { false, Tsl2561::EXP_402 },
    { true,  Tsl2561::EXP_101 },
    { true,  Tsl2561::EXP_402 }  // max
  };

  Serial.printf("autoGainCheck(gain=%u, expo=%u, full=%u, ir=%u, ms=%04x, now=%04x, retry=%u\n", gain, exposure, full, ir, setSensMs, (uint16_t)(millis() & 0xffff), retryOnSaturated);

  // first measurement -> get gain and exposure
  if( setSensMs == 0 ) {
    if( (setSensMs = millis() & 0xffff) == 0 ) setSensMs = 0xffff;

    // get current sensitivity
    if( !tsl.getSensitivity(gain, exposure) ) {
      return false; // I2C error
    }
  }

  // find index of current sensitivity
  uint8_t curr = 0;
  while( curr < sizeof(sensitivity)/sizeof(sensitivity[0]) ) {
    if( sensitivity[curr].gain == gain && sensitivity[curr].exposure == exposure ) {
      break;
    }
    curr++;
  }
  if( curr == sizeof(sensitivity)/sizeof(sensitivity[0]) ) {
    return false; // invalid gain/exposure - should not happen...
  }

  // return (to wait for next sample), or get values and adjust sensitivity if needed
  if( (millis() & 0xffff) - setSensMs < getDelay(exposure) ) {
    full = ir = 0; // indicates no valid values, yet
    return true;
  }
  else {
    setSensMs = millis() & 0xffff;
  }

  if( !tsl.fullLuminosity(full) || !tsl.irLuminosity(ir) ) {
    return false; // I2C error
  }

  uint16_t limit = getLimit(exposure);
  if( full >= 1000 && full <= limit ) {
    // Serial.printf("autoGain normal full=%u, limits=1000-%u, curr=%u\n", full, limit, curr);
    retryOnSaturated = 0;
    return true; // new value within limits of good accuracy
  }

  // adjust sensitivity, if possible
  if( (full < 1000 && ++curr < sizeof(sensitivity)/sizeof(sensitivity[0]))
   || (full > limit && curr-- > 0) ) {
    // Serial.printf("autoGain adjust full=%u, limits=1000-%u, curr=%u\n", full, limit, curr);
    if( !tsl.setSensitivity(sensitivity[curr].gain, sensitivity[curr].exposure) ) {
      return false; // I2C error
    }
    gain = sensitivity[curr].gain;
    exposure = sensitivity[curr].exposure;
    full = ir = 0;
  }
  else {
    // sensitivity already is at minimum or maximum
    if( ++curr > 0 ) {
      retryOnSaturated = 0; // too dark, can't do better, so use what we have
    }
    else {
      retryOnSaturated--; // too bright, might be sensor glitch, so retry
      // Serial.printf("autoGain limit full=%u, ir=%u, limits=1000-%u, curr=%u, retry=%u\n", full, ir, limit, curr, retryOnSaturated);
    }
  }

  return true;
}

// Repeat measurements until either an error occurs or retry count reached 0 or valid values available
bool autoGain( Tsl2561 &tsl, bool &gain, Tsl2561::exposure_t &exposure, uint16_t &full, uint16_t &ir ) {
  uint16_t ms = 0;
  uint8_t retry = 10;
  bool result;

  while( (result = autoGainCheck(tsl, gain, exposure, full, ir, ms, retry)) && ((!full && !ir) || retry) ) {
    waitNext(exposure);
  }

  return result;
}

// Measurement is up to 20% too high for temperatures above 25°C. Compensate for that.
bool compensateTemperature( int16_t centiCelsius, uint32_t &full, uint32_t &ir ) {
  // assume linear gradient 0% at 25°C to +20% at 70°C
  if( centiCelsius >= -3000 && centiCelsius <= 7000 ) {
    full -= (full * (centiCelsius - 2500)) / (5 * (7000 - 2500));
    ir   -= (ir   * (centiCelsius - 2500)) / (5 * (7000 - 2500));
    return true;
  }

  return false;
}

// Round num after valid digits
uint32_t significance( uint32_t num, uint8_t digits ) {
  uint8_t len = 1;
  uint32_t n = num;
  while( n /= 10 ) {
    len++;
  }

  uint32_t e10 = 1;
  while( len-- > digits ) {
    e10 *= 10;
  }

  return ((num + e10 / 2) / e10) * e10;
}

// Calculate lux from raw luminosity values
bool milliLux( uint32_t full, uint32_t ir, uint32_t &mLux, bool csType, uint8_t digits ) {
  if( !full ) {
    mLux = 0;
    return true;
  }

  if( full == 0xffffffff || ir == 0xffffffff ) {
    mLux = 99999999; // indicates saturation at shortest exposure
    return true;
  }

  uint32_t milliRatio = ir * 1000 / full;

  if( csType ) {
    if(       milliRatio <  130 ) { mLux = 1000 * full -  840 * ir; }
    else if ( milliRatio <  260 ) { mLux = 1080 * full - 1380 * ir; }
    else if ( milliRatio <  390 ) { mLux = 1160 * full - 1690 * ir; }
    else if ( milliRatio <  520 ) { mLux = 1260 * full - 1940 * ir; }
    else if ( milliRatio <  650 ) { mLux =  730 * full -  930 * ir; }
    else if ( milliRatio <  800 ) { mLux =  500 * full -  575 * ir; }
    else if ( milliRatio < 1300 ) { mLux =  110 * full -   85 * ir; }
    else {
      mLux = 0;
      return false;
    }
    mLux /= 400 * 16 / 200; // 32 = counts/lux (cpl)
  }
  else {
    if(       milliRatio <  125 ) { mLux = 1000 * full -  895 * ir; }
    else if ( milliRatio <  250 ) { mLux = 1070 * full - 1045 * ir; }
    else if ( milliRatio <  375 ) { mLux = 1150 * full - 1790 * ir; }
    else if ( milliRatio <  500 ) { mLux = 1260 * full - 2050 * ir; }
    else if ( milliRatio <  610 ) { mLux =  740 * full - 1020 * ir; }
    else if ( milliRatio <  800 ) { mLux =  420 * full -  500 * ir; }
    else if ( milliRatio < 1300 ) { mLux =   48 * full -   37 * ir; }
    else {
      mLux = 0;
      return false;
    }
    mLux /= 400 * 16 / 193; // 33 = counts/lux (cpl)
  }

  mLux = significance(mLux, digits); // only the first 4 digits seem to make sense.

  return true;
}

} // namespace Tsl2561Util
