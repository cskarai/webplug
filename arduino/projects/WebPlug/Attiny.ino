#include "WebPlug.h"
#include "Attiny.h"
#include "ISP.h"
#include "attiny/attiny_halo.h"
#include "attinycmd.h"

ISP isp;
Attiny attiny;

#define ATTINY84_DEVICE_ID  0x1E930C
#define ATTINY84_PAGE_SIZE  64

#define DATA_READ_INTERVAL  5
#define HEARTBEAT_INTERVAL  1000

#define ESP_SPI_MIN_CYCLES  11000

#define MINP_OFFSET         17

String Attiny::getOscilloscopeData() {
  String s;

  int minv = 0x7FFFFFF;
  int minp = 0;
  
  for( int st = -49 + MINP_OFFSET; st > -96 + MINP_OFFSET; st-- ) {
    int loc = adcCounter + st;
    if( loc < 0 )
      loc += ADC_BUFFER;

    if( adcData[loc] < minv ) {
      minv = adcData[loc];
      minp = loc;
    }
  }

  bool hasZeroCross = false;
  if( minv < 0 ) {
    for( int i=0; i < 25; i++ ) {
      int loc = minp + i;
      if( loc >= ADC_BUFFER )
        loc -= ADC_BUFFER;

      if( adcData[loc] >= 0 ) {
        int prev = loc - 1;
        if( prev < 0 )
          prev += ADC_BUFFER;
        if( -adcData[prev] < adcData[loc] )
          loc--;
        minp = loc - 24;
        hasZeroCross = true;
        break;
      }
    }
  }

  if( !hasZeroCross ) {
      minp -= MINP_OFFSET;
  }
  if( minp < 0 )
    minp += ADC_BUFFER;

  for( int i=0; i < 49; i++ ) {
    if( i != 0 )
      s += ',';
      
    s += adcData[minp++];
    if( minp == ADC_BUFFER )
      minp = 0;
  }

  return s;
}

void Attiny::uploadAttinyProgram()
{
  if( ! isp.enterProgrammingMode() ) {
    DBG("Can't enter into programming mode!\n");
    return;
  }

  uint32_t device_id = isp.readDeviceID();
  if( device_id != ATTINY84_DEVICE_ID ) {
    DBG("Invalid device ID:%X\n", device_id);
    return;
  }

  if( isp.verifyFlash(attiny_code, sizeof(attiny_code), 0x0000 ) ) {
    DBG("Code doesn't match, needs to be reprogrammed!\n");

    if( ! isp.chipErase() ) {
      DBG("Error at erasing chip!\n");
      return;
    }

    uint16_t programmed = 0;
    while( programmed < sizeof(attiny_code) )
    {
      uint8_t pgmbuf[ATTINY84_PAGE_SIZE];
      memset(pgmbuf, 0xFF, ATTINY84_PAGE_SIZE);

      uint16_t pgmlen = sizeof(attiny_code) - programmed;
      if( pgmlen > ATTINY84_PAGE_SIZE )
        pgmlen = ATTINY84_PAGE_SIZE;

      memcpy(pgmbuf, attiny_code + programmed, pgmlen);

      if( ! isp.writeFlashPage(pgmbuf, ATTINY84_PAGE_SIZE, programmed) ) {
        DBG("Error at flashing code!\n");
        return;
      }

      programmed += ATTINY84_PAGE_SIZE;
    }
  } else {
    DBG("Code matches with the existing one!\n");
  }

  if( ! isp.leaveProgrammingMode() ) {
    DBG("Can't leave programming mode!\n");
    return;
  }
}

void Attiny::start()
{
  SPI.setDataMode(0);
  SPI.setBitOrder(MSBFIRST);

  SPI.begin();
  SPI.setFrequency(100000);

  isp.begin(RESET_PIN, SPI);

  uploadAttinyProgram();  
}

void Attiny::sendCommand(uint16_t command) {
  commandBuffer[commandHead++] = command;
  if( commandHead == ATTINY_COMMAND_BUF ) {
    commandHead = 0;
  }
}

bool Attiny::canSendSPI() {
  uint32_t cycles = ESP.getCycleCount();
  if( cycles - lastSPICycleCount < ESP_SPI_MIN_CYCLES )
    return false;

  lastSPICycleCount = cycles;
  return true;
}

void Attiny::processSPITransfer() {
  // transmit 2nd half of the command
  if( commandSecond >= 0 ) {
    if( canSendSPI() )
    {
      response |= ((uint16_t)SPI.transfer( commandSecond ) << 8);
      commandSecond = -1;
      processResponse( response );
    }
    return;
  }

  // is there a command to transmit?
  if( commandHead == commandTail )
    return;

  // if SPI is not available
  if( canSendSPI() )
  {
    response = SPI.transfer( commandBuffer[commandTail] & 255 );
    commandSecond = commandBuffer[commandTail++] >> 8;

    if( commandTail == ATTINY_COMMAND_BUF )
      commandTail = 0;
  }
}

void Attiny::processResponse(uint16_t resp) {
  if( resp == RES_NOP ) {
    return; // no more data
  }
  if( resp == RES_OVERFLOW_ERROR ) {
    DBG("Overflow error\n");
  }

  // handle ADC result
  if( (resp & 0xC000) == RES_ADC_RESULT ) {
    resp = resp & 0x3FFF;
    if( resp & 0x2000 ) {
      resp |= 0XC000;
    }
    adcData[ adcCounter++ ] = resp;
    if( adcCounter == ADC_BUFFER )
      adcCounter = 0;
  }

  // has more to receive
  if( commandHead == commandTail )
    sendCommand(CMD_NOP);
}

void Attiny::loop()
{
  uint32_t ts = millis();

  if(( ts - lastDataReadMillis ) > DATA_READ_INTERVAL) {
    lastDataReadMillis = ts;
    sendCommand(CMD_NOP);
    sendCommand(CMD_NOP);
  }
  if(( ts - lastHeartbeatMillis ) > HEARTBEAT_INTERVAL )
  {
    lastHeartbeatMillis = ts;
    sendCommand(CMD_REQUEST_HEARTBEAT);
  }

  processSPITransfer();
}

