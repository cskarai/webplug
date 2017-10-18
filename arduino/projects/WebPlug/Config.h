#ifndef CONFIG_H
#define CONFIG_H

#include <inttypes.h>
#include <WebServerCommon.h>

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
};

extern Config config;

#endif /* CONFIG_H */

