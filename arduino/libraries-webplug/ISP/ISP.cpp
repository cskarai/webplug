#include "ISP.h"

#define MAX_TRIES             10
#define MAX_READY_WAIT       100

#define DELAY_BEFORE_RESET    35
#define DELAY_AFTER_RESET     60
#define DELAY_AFTER_ERROR    100
#define DELAY_READY_WAIT       1

#define ISP_ECHO_RESPONSE   0x53

#define ISP_ENTER_PROGRAMMING_MODE              0xAC530000L
#define ISP_CHIP_ERASE                          0xAC800000L
#define ISP_POLL_RDY_BSY                        0xF0000000L

#define ISP_LOAD_EXTENDED_ADDRESS_BYTE          0x4D000000L
#define ISP_LOAD_PROGRAM_MEMORY_PAGE_HIGH       0x48000000L
#define ISP_LOAD_PROGRAM_MEMORY_PAGE_LOW        0x40000000L
#define ISP_LOAD_EEPROM_MEMORY_PAGE             0xC1000000L

#define ISP_READ_PROGRAM_MEMORY_HIGH            0x28000000L
#define ISP_READ_PROGRAM_MEMORY_LOW             0x20000000L
#define ISP_READ_EEPROM_MEMORY                  0xA0000000L
#define ISP_READ_LOCK_BITS                      0x58000000L
#define ISP_READ_SIGNATURE_HIGH                 0x30000000L
#define ISP_READ_SIGNATURE_MIDDLE               0x30000100L
#define ISP_READ_SIGNATURE_LOW                  0x30000200L
#define ISP_READ_FUSE_BITS                      0x50000000L
#define ISP_READ_FUSE_BITS_HIGH                 0x58080000L
#define ISP_READ_FUSE_BITS_EXTENDED             0x50080000L
#define ISP_READ_CALIBRATION_BYTE               0x38000000L

#define ISP_WRITE_PROGRAM_MEMORY_PAGE           0x4C000000L
#define ISP_WRITE_EEPROM_MEMORY                 0xC0000000L
#define ISP_WRITE_EEPROM_MEMORY_PAGE            0xC2000000L
#define ISP_WRITE_LOCK_BITS                     0xACE00000L
#define ISP_WRITE_FUSE_BITS                     0xACA00000L
#define ISP_WRITE_FUSE_BITS_HIGH                0xACA80000L
#define ISP_WRITE_FUSE_BITS_EXTENDED            0xACA40000L

void ISP::begin(uint8_t reset_pin, SPIClass &spi)
{
  this->spi = &spi;
  this->reset_pin = reset_pin;

  pinMode(reset_pin, OUTPUT);

  setReset(HIGH);
}

void ISP::setReset(uint8_t state)
{
  digitalWrite(reset_pin, state);
}

uint32_t ISP::spiTransaction(uint32_t data, uint8_t bytes)
{
  uint32_t result = 0;

  for(uint8_t i=0; i < bytes; i++) {
	  result <<= 8;

	  uint8_t dr = (data >> ((bytes - 1 -i) << 3)) & 0xFF;
	  uint8_t rs = spi->transfer(dr);
	  result |= rs;
  }

  return result;
}

uint8_t ISP::ispTransaction(uint32_t data) {
  uint32_t result = spiTransaction(data);
  return (uint8_t)result;
}

uint8_t ISP::enterProgrammingMode()
{
  for( uint8_t t = 0; t < MAX_TRIES; t++ )
  {
    delay(DELAY_BEFORE_RESET);

    setReset(LOW);
    delay(DELAY_AFTER_RESET);

    uint32_t response = spiTransaction( ISP_ENTER_PROGRAMMING_MODE );

    if(( (response >> 8 ) & 0xFF ) == ISP_ECHO_RESPONSE ) {
      in_programming = 1;
      return 1; // ok
    }

    setReset(HIGH);

    delay(DELAY_AFTER_ERROR);
  }
  return 0; // can't enter into programming mode
}

uint8_t ISP::leaveProgrammingMode()
{
  setReset(HIGH);
  delay(DELAY_BEFORE_RESET);
  in_programming = 0;
  return 1;
}

uint32_t ISP::readDeviceID()
{
  if( ! in_programming )
    return 0xFFFFFFFF;

  uint8_t high = ispTransaction(ISP_READ_SIGNATURE_HIGH);
  uint8_t middle = ispTransaction(ISP_READ_SIGNATURE_MIDDLE);
  uint8_t low = ispTransaction(ISP_READ_SIGNATURE_LOW);
  
  return (((uint32_t)high) << 16) | (((uint32_t)middle) << 8) | low;
}

uint8_t ISP::isBusy()
{
  return ispTransaction(ISP_POLL_RDY_BSY) & 1;
}

uint8_t ISP::waitReady()
{
  if( ! in_programming )
	return 0;

  int readyTries = MAX_READY_WAIT;

  while(readyTries--) {
	if( !isBusy() ) {
	  return 1;
	}
	delay(DELAY_READY_WAIT);
  }
  return 0;
}

uint8_t ISP::readEEPROM(uint16_t addr)
{
  if( ! in_programming )
	return 0xFF;

  return ispTransaction(ISP_READ_EEPROM_MEMORY + (((uint32_t)addr) << 8));
}

uint8_t ISP::readEEPROM(uint8_t * bytes, uint16_t len, uint16_t address)
{
  if( ! in_programming )
	return 0;

  for(uint16_t i=0; i < len; i++) {
	  bytes[i] = readEEPROM(address+i);
  }

  return 1;
}

uint8_t ISP::writeEEPROM(uint16_t addr, uint8_t data)
{
  if( ! in_programming )
	  return 0;

  ispTransaction(ISP_WRITE_EEPROM_MEMORY + (((uint32_t)addr) << 8) + data);
  return waitReady();
}

uint8_t ISP::writeEEPROM(const uint8_t * bytes, uint16_t len, uint16_t address)
{
  if( ! in_programming )
	return 0;

  for(uint16_t i=0; i < len; i++) {
	if( ! writeEEPROM(address+i, bytes[i]) )
	  return 0;
  }

  return 1;
}

uint8_t ISP::readCalibrationByte()
{
  if( ! in_programming )
	return 0xFF;

  return ispTransaction(ISP_READ_CALIBRATION_BYTE);
}

uint8_t ISP::readFlash(uint8_t * bytes, uint16_t len, uint32_t address)
{
  if( ! in_programming )
    return 0;
  if(( len & 1 ) || ( address & 1 ))
    return 0;

  uint16_t current_high = 0xFFFF;
  for( uint16_t i=0; i < len; i+=2) {
    uint32_t addr_read = (address + i) >> 1;
    uint16_t addr_low = (uint16_t)(addr_read & 0xFFFF);
    uint16_t addr_high = (uint16_t)((addr_read >> 16) & 0xFFFF);

    if( current_high != addr_high )
    {
      ispTransaction(ISP_LOAD_EXTENDED_ADDRESS_BYTE + (addr_high << 8));
      current_high = addr_high;
    }

    bytes[i]   = ispTransaction(ISP_READ_PROGRAM_MEMORY_LOW + (((uint32_t)addr_low)<<8) );
    bytes[i+1] = ispTransaction(ISP_READ_PROGRAM_MEMORY_HIGH + (((uint32_t)addr_low)<<8) );
  }

  return 1;
}

uint8_t ISP::verifyFlash(const uint8_t * bytes, uint16_t len, uint32_t address)
{
  if( ! in_programming )
    return 0xFF;
  if(( len & 1 ) || ( address & 1 ))
    return 0xFF;

  uint16_t current_high = 0xFFFF;
  for( uint16_t i=0; i < len; i+=2) {
    uint32_t addr_read = (address + i) >> 1;
    uint16_t addr_low = (uint16_t)(addr_read & 0xFFFF);
    uint16_t addr_high = (uint16_t)((addr_read >> 16) & 0xFFFF);

    if( current_high != addr_high )
    {
      ispTransaction(ISP_LOAD_EXTENDED_ADDRESS_BYTE + (addr_high << 8));
      current_high = addr_high;
    }

    uint8_t b0 = ispTransaction(ISP_READ_PROGRAM_MEMORY_LOW + (((uint32_t)addr_low)<<8) );
    uint8_t b1 = ispTransaction(ISP_READ_PROGRAM_MEMORY_HIGH + (((uint32_t)addr_low)<<8) );

    if(( b0 != bytes[i] ) || ( b1 != bytes[i+1] )) {
      return 1;
    }
  }

  return 0;
}

uint32_t ISP::getFlashSize(uint32_t device_id)
{
  return 1 << (((device_id >> 8) & 0x0F) + 10);
}

uint8_t ISP::chipErase()
{
  if( ! in_programming )
    return 0;

  ispTransaction(ISP_CHIP_ERASE);
  return waitReady();
}

uint8_t ISP::writeFlashPage(const uint8_t * bytes, uint16_t pageSize, uint32_t addr)
{
  if( ! in_programming )
    return 0;

  if( (pageSize == 0) || ( ( pageSize & (pageSize-1) ) != 0 ) )
	return 0;

  ispTransaction(ISP_LOAD_EXTENDED_ADDRESS_BYTE + ((addr >> 17) << 8));
  for( uint16_t i=0; i < pageSize; i+=2) {
    ispTransaction(ISP_LOAD_PROGRAM_MEMORY_PAGE_LOW + (((uint32_t)i)<<7) + bytes[i] );
    ispTransaction(ISP_LOAD_PROGRAM_MEMORY_PAGE_HIGH + (((uint32_t)i)<<7) + bytes[i+1] );
  }

  ispTransaction( ISP_WRITE_PROGRAM_MEMORY_PAGE + ((addr & 0x1FFFE) << 7) );
  return waitReady();
}

uint32_t ISP::readFuses()
{
  if( ! in_programming )
    return 0xFFFFFFFF;

  uint8_t extended = ispTransaction(ISP_READ_FUSE_BITS_EXTENDED);
  uint8_t high = ispTransaction(ISP_READ_FUSE_BITS_HIGH);
  uint8_t low = ispTransaction(ISP_READ_FUSE_BITS);

  return (((uint32_t)extended) << 16) | (((uint32_t)high) << 8) | low;
}

uint16_t ISP::readLockBits()
{
  if( ! in_programming )
    return 0xFFFF;

  return ispTransaction(ISP_READ_LOCK_BITS);
}

uint8_t ISP::writeLockBits(uint8_t lock_bits)
{
  if( ! in_programming )
    return 0;

  ispTransaction(ISP_WRITE_LOCK_BITS + lock_bits);
  return waitReady();
}

uint8_t ISP::writeFuses(uint32_t fuses) {
  if( ! in_programming )
    return 0;

  uint8_t extended = (fuses >> 16) & 0xFF;
  uint8_t high = (fuses >> 8) & 0xFF;
  uint8_t low = fuses & 0xFF;

  ispTransaction(ISP_WRITE_FUSE_BITS + low);
  if( !waitReady() )
	return 0;

  ispTransaction(ISP_WRITE_FUSE_BITS_HIGH + high);
  if( !waitReady() )
	return 0;

  ispTransaction(ISP_WRITE_FUSE_BITS_EXTENDED + extended);
  if( !waitReady() )
	return 0;

  return 1;
}
