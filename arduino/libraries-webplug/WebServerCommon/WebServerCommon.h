#ifndef __WebServerCommon_H__
#define __WebServerCommon_H__

#include <ESPAsyncWebServer.h>

class WebServerConfig
{
private:
  int      wifi_op_mode;
  char     hostname[32];
  char     ssid[32];
  char     password[64];
  char     apssid[32];
  char     sntpServer[32];
  int8_t   sntpTimezoneOffset;

public:
  int      getWifiOpMode() {return wifi_op_mode;}
  void     setWifiOpMode(int mode) {wifi_op_mode = mode;}

  char *   getHostName() { return hostname; }
  void     setHostName(const char *name) { strcpy( hostname, name); }

  char *   getSSID() { return ssid; }
  void     setSSID(const char *ssidIn) { strcpy( ssid, ssidIn); }

  char *   getPassword() { return password; }
  void     setPassword(const char *passwd) { strcpy( password, passwd); }

  char *   getAPSSID() { return apssid; }
  void     setAPSSID(const char *apssidIn) { strcpy( apssid, apssidIn); }

  char *   getSNTPServer() { return sntpServer; }
  void     setSNTPServer(const char *name) { strcpy( sntpServer, name); }

  int8_t   getSNTPTimezoneOffset() { return sntpTimezoneOffset; }
  void     setSNTPTimezoneOffset(int8_t offset) { sntpTimezoneOffset = offset; }
};


enum {
  EVNT_OTA_STARTING_REFRESH,
  EVNT_OTA_DONE,
  EVNT_OTA_HEARTBEAT,
  EVNT_OTA_ERROR,
  EVNT_STARTING_WEB_SERVER,
  EVNT_STARTING_WIFI_AS_STATION,
  EVNT_STARTING_WIFI_AS_SOFTAP,
  EVNT_CONNECTING,
  EVNT_ERROR_CONNECTING_AS_STATION,
  EVNT_WIFI_STATION_IP,
  EVNT_WIFI_SOFTAP_IP,
  EVNT_CONFIG_CHANGED,
  EVNT_CONFIG_SET_AP_NAME,
  EVNT_CONFIG_SET_STA_NAME,
  EVNT_CONFIG_SET_STA_PASSWORD,
  EVNT_CONFIG_SET_HOSTNAME,
  EVNT_CONFIG_SET_OP_MODE,
  EVNT_CONFIG_SET_CHANNEL,
  EVNT_CONFIG_SET_AP_CHANNEL,
  EVNT_CONFIG_SNTP_SET_SERVER,
  EVNT_CONFIG_SNTP_SET_OFFSET,
} WebServerEvents;

class WebServerCommon : public AsyncWebServer
{
private:
  static WebServerCommon * instance;

  WebServerConfig & config;

  static void handleSettingsJsonGet(AsyncWebServerRequest *request);
  static void handleSettingsJsonPost(AsyncWebServerRequest *request);

  static void handleNTPSettingsJsonGet(AsyncWebServerRequest *request);
  static void handleNTPDateSettingsJsonGet(AsyncWebServerRequest *request);
  static void handleNTPSettingsJsonPost(AsyncWebServerRequest *request);

  void otaInit();
  void configureNTP();
  
  String getTimeString();
  String getDateString();
  String getDayString();

public: 
  WebServerCommon(WebServerConfig & configIn, int port) : AsyncWebServer(port), config(configIn) {instance = this;}

  virtual void start();
  virtual void loop();

  virtual void webEvent(int evntnum, const char *, ...) {};
  virtual void startServer();
  virtual void resetServer();

  void enableNTP();

  static String createJSONParamResponse();
  static void addJSONParam(String &string, const char * name, const String & value, const char * type);
  static void addJSONParam(String &string, const char * name, const char * value, const char * type);
  static void addJSONParam(String &string, const char * name, int value, const char * type);
  static void addJSONParam(String &string, const char * name, float value, const char * type);
  static void addJSONParam(String &string, const char * name, const char * type);
  static void finishJSONParamResponse(String &string);
  static void replyJSON(AsyncWebServerRequest *request, const String &string);

  static const char * getContentType(const String& path);
  
  static WebServerCommon * getInstance() { return instance; }
};

#endif /* __WebServerCommon_H__ */

