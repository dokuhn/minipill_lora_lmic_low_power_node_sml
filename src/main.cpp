/*
  main.cpp
  Main code where the control takes place
  @author  Leo Korbee (c), Leo.Korbee@xs4all.nl
  @website iot-lab.org
  @license Attribution-ShareAlike 4.0 International (CC BY-SA 4.0)
  Thanks to all the folks who contributed beforme me on this code or libraries.
  Please share you thoughts and comments or you projects with me through e-mail
  or comment on my website

  Please remove all comments on Serial.print() commands to get debug
  information on hardware serial port (9600 baud). Rember that this wil take
  about 2uA in Sleep Mode

  Hardware information at the end of this file.


  @version 2020-10-08
  Added code so you can use ABP instead of OTAA. Please enable DISABLE_JOIN
  in the config.h in the lmic library and change ABP config in secconfig.h

  ======== Wakup by switch/external interrupt ========

  @version 2020-12-18
  Changed the code so the device can wakeup from sleep when pressing a button.
  Checkout the hardware configuration at the end of this file. Please use a
  debounce circuit to avoid multiple interrupts occur at once.

  @version 2021-02-27
  No BME280 connected and using TTN Stack V3 with ABP

  @version 2021-03-07
  LMIC.rxDelay = 5; added in setup function to prevent downlink messages
  It should be 5 seconds, other values do not have the same effect!
*/

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include "STM32LowPower.h"
#include "STM32IntRef.h"

#include "sml.h"
#include <RingBuf.h>

void do_send(osjob_t* j);

// #define Serial Serial2
HardwareSerial Serial2(USART2);   // or HardWareSerial Serial2 (PA3, PA2);


double T1Wh = -2, SumWh = -2;

typedef struct {
  const unsigned char OBIS[6];
  void (*Handler)();
} OBISHandler;


void PowerT1() { smlOBISWh(T1Wh); }

void PowerSum() { smlOBISWh(SumWh); }

// clang-format off
OBISHandler OBISHandlers[] = {
  {{ 0x01, 0x00, 0x01, 0x08, 0x01, 0xff }, &PowerT1},      /*   1-  0:  1.  8.1*255 (T1) */
  // {{ 0x01, 0x00, 0x01, 0x08, 0x00, 0xff }, &PowerSum},     /*   1-  0:  1.  8.0*255 (T1 + T2) */
  {{ 0, 0 }}
};
// clang-format on

#define MAX_BUF_SIZE 1024

sml_states_t currentState;

RingBuf<unsigned char, MAX_BUF_SIZE> myBuffer;

char floatBuffer[20];

void print_buffer(){
  
  unsigned int i = 0;
  unsigned int j = 0;
  char b[5];
  Serial.print(F("Size: "));
  Serial.print(myBuffer.size());
  Serial.println("");
  Serial.println(F("--- "));
  for (j = 0; j < myBuffer.size(); j++) {
    i++;
    sprintf(b, "0x%02X", myBuffer[j]);
    Serial.print(b);
    if (j < myBuffer.size() - 1) {
      Serial.print(", ");
    }
    else {
      Serial.println("");
      Serial.println(F("--- "));
    }
    if ((i % 15) == 0) {
      Serial.println("");
      i = 0;
    }
  }
}

void readByte(unsigned char currentChar)
{
  unsigned int i = 0, iHandler = 0;
  // Serial.printf("%02x",currentChar);
  currentState = smlState(currentChar);
  if (currentState == SML_START) {
    myBuffer.clear();
    myBuffer.push(0x1B);
    myBuffer.push(0x1B);
    myBuffer.push(0x1B);
    /* reset local vars */
    T1Wh = -3;
    // SumWh = -3;
  }
  else {
    if (myBuffer.size() < MAX_BUF_SIZE) {
      myBuffer.push(currentChar);
    }
    else {
      Serial.print(F(">>> Message larger than MAX_BUF_SIZE\n"));
    }
  }
  if(currentState == SML_LISTEND) {
        /* check handlers on last received list */
        for (iHandler = 0; OBISHandlers[iHandler].Handler != 0 &&
                           !(smlOBISCheck(OBISHandlers[iHandler].OBIS));
             iHandler++)
          ;
        if (OBISHandlers[iHandler].Handler != 0) {
          OBISHandlers[iHandler].Handler();
        }
      }
  if (currentState == SML_UNEXPECTED) {
    Serial.print(F(">>> Unexpected byte\n"));
  }
  if (currentState == SML_FINAL) {
    Serial.print(F(">>> Successfully received a complete message!\n"));
    print_buffer();
    
    Serial.print(F("\n"));

    Serial.print(F("Power T1    (1-0:1.8.1)..: "));
    dtostrf(T1Wh, 10, 3, floatBuffer);
    Serial.print(floatBuffer);
    Serial.print(F("\n"));

    // Serial.print(F("Power T1+T2 (1-0:1.8.0)..: "));
    // dtostrf(SumWh, 10, 3, floatBuffer);
    // Serial.print(floatBuffer);
    Serial.print(F("\n\n\n\n"));
  }
  if (currentState == SML_CHECKSUM_ERROR) {
    Serial.print(F(">>> Checksum error.\n"));
  }
}

// include security credentials OTAA, check secconfig_example.h for more information
#include "secconfig.h"

#ifdef DISABLE_JOIN
// These callbacks are only used in over-the-air activation, so they are
// left empty here. We cannot leave them out completely otherwise the linker will complain.
// DISABLE_JOIN is set in config.h for using ABP
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }
#else
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}
#endif

static osjob_t sendjob;

// led for signing a packet is send
#define SIGNAL_LED PA1

// Sleep this many microseconds. Notice that the sending and waiting for downlink
// will extend the time between send packets. You have to extract this time
// #define SLEEP_INTERVAL 300000
#define SLEEP_INTERVAL 300000

// Pin mapping for the MiniPill LoRa with the RFM95 LoRa chip
const lmic_pinmap lmic_pins =
    {
        .nss = PA4,
        .rxtx = LMIC_UNUSED_PIN,
        .rst = PA9,
        .dio = {PA10, PB4, PB5},
};

// event called by os on events
void onEvent(ev_t ev)
{
  Serial.print(os_getTime());
  Serial.print(": ");
  switch (ev)
  {
  case EV_SCAN_TIMEOUT:
    Serial.println(F("EV_SCAN_TIMEOUT"));
    break;
  case EV_BEACON_FOUND:
    Serial.println(F("EV_BEACON_FOUND"));
    break;
  case EV_BEACON_MISSED:
    Serial.println(F("EV_BEACON_MISSED"));
    break;
  case EV_BEACON_TRACKED:
    Serial.println(F("EV_BEACON_TRACKED"));
    break;
  case EV_JOINING:
    Serial.println(F("EV_JOINING"));
    break;
  case EV_JOINED:
    Serial.println(F("EV_JOINED"));

    // Disable link check validation (automatically enabled
    // during join, but not supported by TTN at this time).
    LMIC_setLinkCheckMode(0);
    break;
  case EV_RFU1:
    Serial.println(F("EV_RFU1"));
    break;
  case EV_JOIN_FAILED:
    Serial.println(F("EV_JOIN_FAILED"));
    break;
  case EV_REJOIN_FAILED:
    Serial.println(F("EV_REJOIN_FAILED"));
    break;
    break;
  case EV_TXCOMPLETE:
    Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
    if (LMIC.txrxFlags & TXRX_ACK)
      Serial.println(F("Received ack"));
    if (LMIC.dataLen)
    {
      Serial.print(F("Received "));
      Serial.print(LMIC.dataLen);
      Serial.println(F(" bytes of payload"));
      // print data
      for (int i = 0; i < LMIC.dataLen; i++)
      {
        Serial.print(LMIC.frame[LMIC.dataBeg + i], HEX);
      }
      Serial.println();
    }
    Serial.println("going to sleep");
    // reset low power parameters
    LowPower.begin();
    delay(100);
    // set PA6 to analog to reduce power due to currect flow on DIO on BME280
    pinMode(PA6, INPUT_ANALOG);
    // take SLEEP_INTERVAL time to sleep
    LowPower.deepSleep(SLEEP_INTERVAL);

    // Serial.println("queing next job");
    // next transmission
    do_send(&sendjob);

    break;
  case EV_LOST_TSYNC:
    Serial.println(F("EV_LOST_TSYNC"));
    break;
  case EV_RESET:
    Serial.println(F("EV_RESET"));
    break;
  case EV_RXCOMPLETE:
    // data received in ping slot
    Serial.println(F("EV_RXCOMPLETE"));
    break;
  case EV_LINK_DEAD:
    Serial.println(F("EV_LINK_DEAD"));
    break;
  case EV_LINK_ALIVE:
    Serial.println(F("EV_LINK_ALIVE"));
    break;
  default:
    Serial.println(F("Unknown event"));
    break;
  }
}

//
void do_send(osjob_t* j)
{
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND)
  {
    Serial.println(F("OP_TXRXPEND, not sending"));
  } else
  {
    uint32_t timeout_cnt = 0;
    Serial.print(os_getTime());
    Serial.print(": ");
    Serial.print(F("beginning to read SML ..."));
    while( (Serial2.available() <= 0) && (timeout_cnt <= 500) ) {
      // __asm__("nop\n\t");
      delay(10);
      timeout_cnt += 1;
    }

    if (timeout_cnt > 500)
    {
      Serial.print(F("timeout while reading!!\n"));
    }
    else
    {
      while (Serial2.available() > 0)
      {
        readByte(Serial2.read());
      }

      

      Serial.print(F("end of reading SML!!"));
    }

    // Prepare upstream data transmission at the next possible time.
    // uint8_t dataLength = 2;
    // uint8_t data[dataLength];

    // read vcc and add to bytebuffer
    // int32_t vcc = IntRef.readVref();
    // data[0] = (vcc >> 8) & 0xff;
    // data[1] = (vcc & 0xff);

    LMIC_setTxData2(1, (uint8_t* )floatBuffer, sizeof(floatBuffer), 0);
    Serial.println(F("Packet queued"));
    // signal with LED that data is queued
    digitalWrite(SIGNAL_LED, LOW);
    delay(1000);
    digitalWrite(SIGNAL_LED, HIGH);

  }
}

void setup()
{
  Serial.begin(9600);
  Serial2.begin(9600);
  // delay at startup for debugging reasons
  delay(8000);
  Serial.println(F("Starting"));

  // set pin as OUPUT for LED
  pinMode(SIGNAL_LED, OUTPUT);
  digitalWrite(SIGNAL_LED, HIGH);

  // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();
  // to incrise the size of the RX window.
  LMIC_setClockError(MAX_CLOCK_ERROR * 25 / 100);

  // Set static session parameters when using ABP.
  // Instead of dynamically establishing a session
  // by joining the network, precomputed session parameters are be provided.
  #ifdef DISABLE_JOIN
  uint8_t appskey[sizeof(APPSKEY)];
  uint8_t nwkskey[sizeof(NWKSKEY)];
  memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
  memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
  LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);
  // These settings are needed for correct communication. By removing them you
  // get empty downlink messages
  // Disable ADR
  LMIC_setAdrMode(0);
  // TTN uses SF9 for its RX2 window.
  LMIC.dn2Dr = DR_SF9;
  // prevent some downlink messages, TTN advanced setup is set to the default 5
  LMIC.rxDelay = 5;
  // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
  LMIC_setDrTxpow(DR_SF12,14);
  #endif

  // Configure low power at startup
  LowPower.begin();

  // Start job (sending automatically starts OTAA too, or first ABP message)
  do_send(&sendjob);
}

void loop()
{
  // run the os_loop to check if a job is available
  os_runloop_once();
}

/* MiniPill LoRa v1.x mapping - LoRa module RFM95W
  PA4  // SPI1_NSS   NSS  - RFM95W
  PA5  // SPI1_SCK   SCK  - RFM95W
  PA6  // SPI1_MISO  MISO - RFM95W
  PA7  // SPI1_MOSI  MOSI - RFM95W

  PA10 // USART1_RX  DIO0 - RFM95W
  PB4  //            DIO1 - RFM95W
  PB5  //            DIO2 - RFM95W

  PB6  // USART1_TX
  PB7  // USART1_RX 

  PA9  // USART1_TX  RST  - RFM95W

  VCC - 3V3
  GND - GND

  PB11 // switch connected to ground with little debounce circuit:

  Schematic:

  VCC
   |
  | | internal PULL_UP ristor, datasheet: about 40k
  | |
   |
  -+--- PB11 ---+---[ 4k7 ]----+
                |              |
                |              +- |
              _____               |-|
              _____ 100n       +- |
                |              |
  GND-----------+--------------+

*/
