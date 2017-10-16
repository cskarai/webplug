#include "ISP.h"

/*******************************************

Arduino Default pins:
  D10 <=> RESET
  D11 <=> MOSI
  D12 <=> MISO
  D13 <=> SCK

ESP8266 Default pins:
  D1  <=> RESET
  D7  <=> MISO
  D6  <=> MOSI
  D5  <=> SCK

********************************************/

#ifdef ESP8266
#define RESET_PIN  5
#else
#define RESET_PIN 10
#endif

ISP isp;

void setup() {
  Serial.begin(115200);
  Serial.println(F("ISP device ID reader"));

  SPI.setDataMode(0);
  SPI.setBitOrder(MSBFIRST);
  
#ifndef ESP8266
  // Clock Div can be 2,4,8,16,32,64, or 128
  SPI.setClockDivider(SPI_CLOCK_DIV128);
#endif
  SPI.begin();
#ifdef ESP8266
  SPI.setFrequency(100000);
#endif

  isp.begin(RESET_PIN, SPI);

  if( ! isp.enterProgrammingMode() ) {
    Serial.println(F("Can't enter into programming mode!"));
    return;
  }

  uint32_t device_id = isp.readDeviceID();
  Serial.print(F("Device ID:"));
  Serial.println(device_id, HEX);

  uint32_t flash_size = isp.getFlashSize( device_id ); 
  Serial.print(F("Flash size:"));
  Serial.println(flash_size);

  if( ! isp.leaveProgrammingMode() ) {
    Serial.println(F("Can't leave programming mode!"));
    return;
  }
}

void loop() {
  // do nothing
}

