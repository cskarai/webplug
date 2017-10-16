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

uint8_t test_data [] = {
  0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
  0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
};

ISP isp;

void eeprom_dump() {
  Serial.print(F("Dumping EEPROM from 0x0000 address:"));

  uint8_t eeprom_data[16];
  if( ! isp.readEEPROM(eeprom_data, 16, 0x0000) )
  {
    Serial.println(F("Can't load EEPROM data!"));
    return;
  }

  for(uint8_t i=0; i < 16; i++)
  {
    if(( i & 7 ) == 0) {
      Serial.println();
      Serial.print(F("0x"));
      Serial.print(i, HEX);
      Serial.print(F("  "));
    }
    char buf[20];
    sprintf(buf, "%02x ", eeprom_data[i]);
    Serial.print(buf);
  }
  Serial.println();
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("ISP EEPROM programming sample"));

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

  if( !isp.writeEEPROM( test_data, 16, 0x0000 ) ) {
    Serial.println(F("Can't fill EEPROM data!"));
    return;
  }

  eeprom_dump();

  if( ! isp.leaveProgrammingMode() ) {
    Serial.println(F("Can't leave programming mode!"));
    return;
  }
}

void loop() {
  // do nothing
}

