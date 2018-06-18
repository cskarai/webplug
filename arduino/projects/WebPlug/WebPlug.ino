#include <WebServerCommon.h>
#include <RCSwitch.h>
#include "WPWebServer.h"
#include "WebPlug.h"
#include "Attiny.h"
#include "ISP.h"
#include "attinycmd.h"
#include "RF.h"
#include "Config.h"

/*******************************************

ESP8266 <=> Attiny85 pin configuration:

    D0  <=> RESET (Attiny)
    D1  <=> RELAY_1
    D2  <=> RF RECEIVER
    D5  <=> SCK
    D6  <=> MOSI
    D7  <=> MISO
    D8  <=> RELAY_2

********************************************/

WPWebServer webPlugWebServer(80);

void setup() {
  DBG_INIT();
  DBG("WebPlug startup\n");

  config.init();

  // configure mains controller
  pinMode(RELAY_1, OUTPUT);
  pinMode(RELAY_2, OUTPUT);
  digitalWrite(RELAY_1, LOW);
  digitalWrite(RELAY_2, LOW);

  webPlugWebServer.start();
  attiny.start();
  rf.start();

  delay(200); // wait attiny to start
  attiny.sendCommand(CMD_ADC_START);
}

void loop() {
  webPlugWebServer.loop();
  attiny.loop();
  rf.loop();
}
