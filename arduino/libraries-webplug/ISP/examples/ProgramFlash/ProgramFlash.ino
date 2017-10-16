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

uint8_t neutralAssemblyData[] = {
    0x11, 0x24, // eor r1, r1
    0x00, 0x00, // nop
    0x00, 0xc0, // rjmp .+0
    0x81, 0xe0, // ldi r24, 0x01
    0x90, 0xe0, // ldi r25, 0x00
    0x6a, 0x95, // dec r22
    0x01, 0xf4, // brne .+0
    0x00, 0x2c, // mov r0, r0
};

ISP isp;

// this method works in the most cases,
// although it's incorrect for certain chips
uint16_t getPageSize(uint32_t flashSize) {
  switch(flashSize)
  {
    case 262144:
    case 131072:
    case 65536:
      return 256;
    case 32768:
    case 16384:
      return 128;
    case 8192:
    case 4096:
      return 64;
    case 2048:
    case 1024:
    default:
      return 32;
  }
}

void flashDump() {
  Serial.print(F("Dumping flash from 0x0000 address:"));

  uint8_t flash_data[16];
  if( ! isp.readFlash(flash_data, sizeof(flash_data), 0x0000) )
  {
    Serial.println(F("Can't load flash data!"));
    return;
  }

  for(uint16_t i=0; i < sizeof(flash_data); i++)
  {
    if(( i & 7 ) == 0) {
      Serial.println();
      Serial.print(F("0x"));
      Serial.print(i, HEX);
      Serial.print(F("  "));
    }
    char buf[20];
    sprintf(buf, "%02x ", flash_data[i]);
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

  uint32_t deviceId = isp.readDeviceID();
  uint32_t flashSize = isp.getFlashSize( deviceId ); 

  if( ! isp.chipErase() ) {
    Serial.println(F("Can't erase chip!"));
    isp.leaveProgrammingMode();
    return;
  }
  
  flashDump();
  
  uint16_t pageSize = getPageSize(flashSize);
  uint8_t page[pageSize];
  
  for(uint16_t i=0; i < pageSize; i++)
      page[i] = neutralAssemblyData[ i & 15 ];

  if( ! isp.writeFlashPage(page, pageSize, 0x0000) ) {
    Serial.println(F("Can't flash the chip!"));
    isp.leaveProgrammingMode();
    return;
  }
  
  flashDump();

  if( ! isp.leaveProgrammingMode() ) {
    Serial.println(F("Can't leave programming mode!"));
    return;
  }
}

void loop() {
  // do nothing
}

