#include "Config.h"
#include "WebPlug.h"

#define CONFIG_MAGIC                 0xC5AB105C
#define CONFIG_ADDR                  0x3F0000

#define NTP_SERVER                   "hu.pool.ntp.org"

#define DEFAULT_APSSID               "WebPlug"
#define DEFAULT_HOSTNAME             "WebPlug"
#define DEFAULT_SSID                 ""
#define DEFAULT_PASSWORD             ""
#define DEFAULT_OPMODE               SOFTAP_MODE

#define DEFAULT_SNTP_SERVER          "hu.pool.ntp.org"
#define DEFAULT_TIMEZONE_OFFSET      2

#define DEFAULT_CONTROL_PNT          false
#define DEFAULT_FACTOR               0.47f

#define DEFAULT_CURRENT_MULTIPLIER   1.00f
#define DEFAULT_MAINS_VOLTAGE        230

extern "C"{
 #include "user_interface.h"
}

Config config;

void Config::init()
{
  spi_flash_read(CONFIG_ADDR, (uint32_t *)this, sizeof(Config));
  int crc = crc16;
  crc16 = 0;

  int crcCalc = crc16_data((const uint8_t *)this, sizeof(Config), 0);
  if(( magic != CONFIG_MAGIC ) || (crcCalc != crc))
  {
    DBG("Restore %x-%d/%d\n", magic, crcCalc, crc);
    restore();
  }
}

void Config::save()
{
  DBG("Saving config...\n");
  crc16 = 0;
  crc16 = crc16_data((const uint8_t *)this, sizeof(Config), 0);

  spi_flash_erase_sector(CONFIG_ADDR>>12);
  spi_flash_write(CONFIG_ADDR, (uint32_t *)this, sizeof(Config));
}
  
void Config::restore()
{
  memset(this, 0, sizeof(Config));
  magic = CONFIG_MAGIC;

  setSSID(DEFAULT_SSID);
  setPassword(DEFAULT_PASSWORD);
  setAPSSID(DEFAULT_APSSID);
  setHostName(DEFAULT_HOSTNAME);
  setWifiOpMode(DEFAULT_OPMODE);

  setSNTPServer(DEFAULT_SNTP_SERVER);
  setSNTPTimezoneOffset(DEFAULT_TIMEZONE_OFFSET);

  setInterpolationFactor(DEFAULT_FACTOR);
  setInterpolationControlPointDraw(DEFAULT_CONTROL_PNT);

  setCurrentMultiplier(DEFAULT_CURRENT_MULTIPLIER);
  setMainsVoltage(DEFAULT_MAINS_VOLTAGE);
  
  save();
}

uint16_t Config::crc16_add(uint8_t b, uint16_t acc)
{
  acc ^= b;
  acc  = (acc >> 8) | (acc << 8);
  acc ^= (acc & 0xff00) << 4;
  acc ^= (acc >> 8) >> 4;
  acc ^= (acc & 0xff00) >> 5;
  return acc;
}

uint16_t Config::crc16_data(const uint8_t *data, int len, uint16_t acc)
{
  for(int i = 0; i < len; ++i) {
    acc = crc16_add(*data, acc);
    ++data;
  }
  return acc;
}

