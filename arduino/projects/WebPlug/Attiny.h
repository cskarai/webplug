#ifndef ATTINY_H
#define ATTINY_H

#define ATTINY_COMMAND_BUF  16
#define ADC_BUFFER          100

class Attiny
{
private:
  uint16_t  commandBuffer[ATTINY_COMMAND_BUF];
  uint32_t  commandTail = 0;
  uint32_t  commandHead = 0;
  int32_t   commandSecond = -1;

  int16_t   adcData[ADC_BUFFER];
  uint32_t  adcCounter = 0;

  uint32_t  lastSPICycleCount = 0;

  uint16_t  response = 0;

  uint32_t  lastHeartbeatMillis = 0;
  uint32_t  lastDataReadMillis = 0;

  void uploadAttinyProgram();
  bool canSendSPI();
  void processSPITransfer();
  void processResponse(uint16_t resp);

public:
  void start();
  void loop();

  void sendCommand(uint16_t command);

  String getOscilloscopeData();
};

extern Attiny attiny;

#endif

