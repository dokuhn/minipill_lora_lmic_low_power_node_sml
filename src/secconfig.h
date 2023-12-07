/*
  secconfig.h
  configuration file with security information, not shared on the internet.

  THIS IS AN EXAMPLE FILE, PLEASE RENAME TO secconfig.h


  @author  Leo Korbee (c), Leo.Korbee@xs4all.nl
  @website iot-lab.org
  @license Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)
*/

// When Using OTAA as activation type (default)

// This EUI must be in little-endian format, so least-significant-byte
// first. When copying an EUI from ttnctl output, this means to reverse
// the bytes. For TTN issued EUIs the last bytes should be 0xD5, 0xB3,
// 0x70.
static const u1_t PROGMEM APPEUI[8]={ 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x23, 0x45 };

// This should also be in little endian format, see above.
static const u1_t PROGMEM DEVEUI[8]={ 0xD6, 0x33, 0x06, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 };

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
// The key shown here is the semtech default key.
static const u1_t PROGMEM APPKEY[16] = { 0x44, 0x99, 0xB4, 0xD3, 0xDB, 0x02, 0x07, 0xE3, 0xD6, 0x1A, 0x06, 0x6E, 0xCB, 0x26, 0xFC, 0xA2 };


// When using ABP as activation type
// set DISABLE_JOIN in config.h in the lmic library

// LoRaWAN NwkSKey, network session key (TTN msb first)
static const PROGMEM u1_t NWKSKEY[16] = { 0xE0, 0x92, 0xEA, 0x35, 0x7C, 0x80, 0xA8, 0x26, 0xAE, 0xB0, 0x7C, 0xF3, 0xD7, 0xCA, 0xAD, 0x85 };
// LoRaWAN AppSKey, application session key (TTN msn first)
static const u1_t PROGMEM APPSKEY[16] = { 0xB2, 0xB5, 0xC8, 0xDF, 0x37, 0x3F, 0xFF, 0x37, 0x39, 0x86, 0xAD, 0x48, 0x51, 0x96, 0xDD, 0x01 };
// LoRaWAN DevAddr, end-device address (TTN msb first)
static const u4_t DEVADDR = 0x260B5215 ;
