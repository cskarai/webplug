#ifndef __ISP_H__
#define __ISP_H__

#include "SPI.h"

#include <inttypes.h>

class ISP
{
  private:
	SPIClass *spi = 0;

	uint8_t  reset_pin;
    uint8_t  in_programming = 0;

    void     setReset(uint8_t state);

    uint32_t spiTransaction(uint32_t data, uint8_t bytes=4);
    uint8_t  ispTransaction(uint32_t data);

  public:
    void     begin(uint8_t reset_pin, SPIClass & spi = SPI );

    uint8_t  enterProgrammingMode();
    uint8_t  leaveProgrammingMode();

    uint8_t  isBusy();
    uint8_t  waitReady();

    uint8_t  readEEPROM(uint16_t addr);
    uint8_t  readEEPROM(uint8_t * bytes, uint16_t len, uint16_t address);
    uint8_t  writeEEPROM(uint16_t addr, uint8_t data);
    uint8_t  writeEEPROM(const uint8_t * bytes, uint16_t len, uint16_t addr);

    uint32_t readDeviceID();
    uint16_t readLockBits();
    uint8_t  readCalibrationByte();
    uint32_t readFuses();

    uint8_t  writeLockBits(uint8_t lock_bits);
    uint8_t  writeFuses(uint32_t fuses); // use with caution

    uint8_t  chipErase();
    uint8_t  readFlash(uint8_t * bytes, uint16_t len, uint32_t address);
    uint8_t  verifyFlash(const uint8_t * bytes, uint16_t len, uint32_t address);
    uint8_t  writeFlashPage(const uint8_t * bytes, uint16_t pageSize, uint32_t addr);

    uint32_t getFlashSize(uint32_t device_id);
};

#endif /* __ISP_H__ */
