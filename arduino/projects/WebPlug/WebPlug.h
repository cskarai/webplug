#ifndef WEBPPLUG_H
#define WEBPPLUG_H

/*******************************************

ESP8266 <=> Attiny84 pin configuration:

    D0  <=> RESET (Attiny)
    D1  <=> RELAY_1
    D2  <=> RF RECEIVER
    D6  <=> MOSI
    D5  <=> SCK
    D7  <=> MISO
    D8  <=> RELAY_2

********************************************/

#ifndef ESP8266
#error This code requires ESP8266
#endif

#define RESET_PIN           D0
#define RELAY_1             D1
#define RELAY_2             D8
#define RF_RECEIVER         D2

#define DEBUG

#ifdef DEBUG

#define DBG_INIT()       Serial.begin(115200);
#define DBG(format, ...) do { String s = F((format)); Serial.printf(s.c_str(), ## __VA_ARGS__); } while(0)
#define DBG_RAM(format, ...) do { Serial.printf(format, ## __VA_ARGS__); } while(0)

#else

#define DBG_INIT()
#define DBG(format, ...)
#define DBG_RAM(format, ...)

#endif

#endif /* WEBPPLUG_H */


