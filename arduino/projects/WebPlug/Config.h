#ifndef CONFIG_H
#define CONFIG_H

#include <inttypes.h>
#include <WebServerCommon.h>


#define RADIO_CODE_INVALID  0xFFFFFFFF

#define RADIO_CODE_1_ON     0
#define RADIO_CODE_1_OFF    1
#define RADIO_CODE_1_TOGGLE 2
#define RADIO_CODE_2_ON     3
#define RADIO_CODE_2_OFF    4
#define RADIO_CODE_2_TOGGLE 5
#define RADIO_CODE_MAX      6


class Config : public WebServerConfig
{
  private:
    static   uint16_t crc16_add(uint8_t b, uint16_t crc);
    static   uint16_t crc16_data(const uint8_t *data, int datalen, uint16_t acc);

    int      magic;
    int      crc16;

    float    interpolationFactor;
    bool     interpolationControlPointDraw;

    int      mainsVoltage;
    float    currentMultiplier;

    uint32_t radio_codes[6];
    
  public:
    void     init();
    
    void     save();
    void     restore();

  public:
    float    getInterpolationFactor() {return interpolationFactor;}
    void     setInterpolationFactor(float factor) {interpolationFactor = factor;}

    bool     isInterpolationControlPointDraw() {return interpolationControlPointDraw; }
    void     setInterpolationControlPointDraw(int draw) {interpolationControlPointDraw = draw;}

    float    getCurrentMultiplier() { return currentMultiplier; }
    void     setCurrentMultiplier(float multiplier) { currentMultiplier = multiplier; }

    int      getMainsVoltage() { return mainsVoltage; }
    void     setMainsVoltage(int voltage) { mainsVoltage = voltage; }

    uint32_t getRadioCode(int code);
    void     setRadioCode(int code, uint32_t value);
};

extern Config config;

#endif /* CONFIG_H */

