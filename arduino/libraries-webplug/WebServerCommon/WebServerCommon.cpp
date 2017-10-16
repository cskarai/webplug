#include <WebServerCommon.h>

#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

#define FLSTR(s) ((const __FlashStringHelper*)(s))
#define DUMMY_PASSWORD "!#INVL#!"

extern "C"{
 #include "user_interface.h"
}

WebServerCommon * WebServerCommon::instance = 0;

AsyncWebSocket ws("/ws");


void WebServerCommon::otaInit()
{
  // Port defaults to 8266
  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname(config.getHostName());

  ArduinoOTA.onStart([]() {
    WebServerCommon::instance->webEvent(EVNT_OTA_STARTING_REFRESH, "Starting OTA update\n");
  });
  ArduinoOTA.onEnd([]() {
    WebServerCommon::instance->webEvent(EVNT_OTA_DONE, "OTA update done\n");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    WebServerCommon::instance->webEvent(EVNT_OTA_HEARTBEAT, "OTA progress: %d/%d\n", progress, total);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    WebServerCommon::instance->webEvent(EVNT_OTA_ERROR, "OTA error: %d\n", error);
  });
  ArduinoOTA.begin();  
}

void WebServerCommon::start() 
{
  webEvent(EVNT_STARTING_WEB_SERVER, "Starting web server\n");

  if( config.getWifiOpMode() != wifi_get_opmode() )
    wifi_set_opmode( config.getWifiOpMode() );

  int mode = config.getWifiOpMode();

  if( mode & STATION_MODE )
  {
    struct station_config cfg;
    wifi_station_get_config(&cfg);
    wifi_station_set_hostname(config.getHostName());

    webEvent(EVNT_STARTING_WIFI_AS_STATION, "Wifi start as station %s-%s/%s (%d)\n", config.getHostName(), config.getSSID(), config.getPassword(), wifi_get_channel());
    WiFi.begin(config.getSSID(), config.getPassword(), wifi_get_channel());
    
    for(int i=0; i < 80; i++)
    {
      if(WiFi.status() == WL_CONNECTED)
        break;

      int tsi = millis();

      while( millis() - tsi < 250 )
      {
        yield();
        webEvent(EVNT_CONNECTING, ".");
      }
        
      ESP.wdtFeed();
    }

    if( WiFi.status() != WL_CONNECTED ) {
      webEvent( EVNT_ERROR_CONNECTING_AS_STATION, "Error connecting as station\n" );
      mode |= SOFTAP_MODE;
    }
    else
    {
      IPAddress addr = WiFi.localIP();
      String ip = addr.toString();
      webEvent(EVNT_WIFI_STATION_IP, "Wifi station got ip: %s\n", ip.c_str(), (uint32_t)addr);
    }
  }
  if( mode & SOFTAP_MODE )
  {
    struct softap_config apcfg;
    wifi_softap_get_config(&apcfg);

    webEvent(EVNT_STARTING_WIFI_AS_SOFTAP, "Wifi start as ap %s (%d)\n", config.getAPSSID(), apcfg.channel);
    WiFi.softAP(config.getAPSSID(), NULL, apcfg.channel);

    IPAddress addr = WiFi.softAPIP();
    String ip = addr.toString();
    webEvent(EVNT_WIFI_SOFTAP_IP, "Wifi ap ip: %s\n", ip.c_str(), (uint32_t)addr);
  }
  
  otaInit();

  MDNS.addService("http","tcp",80);

  startServer();
}

void WebServerCommon::handleSettingsJsonGet(AsyncWebServerRequest *request)
{
  struct station_config cfg;
  wifi_station_get_config(&cfg);
  struct softap_config apcfg;
  wifi_softap_get_config(&apcfg);
  
  String args = createJSONParamResponse();
  addJSONParam(args, "wifi_host", instance->config.getHostName(), "value");
  addJSONParam(args, "wifi_sta_name", (const char *)cfg.ssid, "value");
  addJSONParam(args, "wifi_sta_passwd", DUMMY_PASSWORD, "value");
  addJSONParam(args, "wifi_channel", wifi_get_channel(), "value");
  addJSONParam(args, "wifi_op_mode", (int)wifi_get_opmode(), "value");
  addJSONParam(args, "wifi_ap_name", (const char *)apcfg.ssid, "value");
  addJSONParam(args, "wifi_ap_channel", apcfg.channel, "value");
  finishJSONParamResponse(args);
  replyJSON(request, args);
}

void WebServerCommon::handleSettingsJsonPost(AsyncWebServerRequest *request)
{
  struct station_config cfg;
  wifi_station_get_config(&cfg);
  struct softap_config apcfg;
  wifi_softap_get_config(&apcfg);
  String wifiHostName;

  bool updateAp = false;
  bool updateSta = false;
  bool updateOp = false;
  bool updateHost = false;
  bool configChanged = false;
  bool reset = false;

  if( request->hasParam("redirect", true) ) {
      request->redirect(request->getParam("redirect", true)->value());
  }
  if( request->hasParam("wifi_ap_name", true) ) {
    const String& value = request->getParam("wifi_ap_name", true)->value();

    if( value != (char *)apcfg.ssid )
    {
      strcpy((char *)apcfg.ssid, value.c_str());
      instance->config.setAPSSID(value.c_str());
      apcfg.ssid_len = value.length();
      instance->webEvent(EVNT_CONFIG_SET_AP_NAME, "AP name changed to %s\n", value.c_str());
      updateAp = true;
    }
  }
  if( request->hasParam("wifi_sta_name", true) ) {
    const String& value = request->getParam("wifi_sta_name", true)->value();

    if( value != (char *)cfg.ssid )
    {
      strcpy((char *)cfg.ssid, value.c_str());
      instance->config.setSSID(value.c_str());
      instance->webEvent(EVNT_CONFIG_SET_STA_NAME, "STA name changed to %s\n", value.c_str());
      updateSta = true;
    }
  }
  if( request->hasParam("wifi_sta_passwd", true) ) {
    const String& value = request->getParam("wifi_sta_passwd", true)->value();

    if( value != DUMMY_PASSWORD )
    {
      strcpy((char *)cfg.password, value.c_str());
      instance->config.setPassword(value.c_str());
      instance->webEvent(EVNT_CONFIG_SET_STA_PASSWORD, "STA password changed to %s\n", value.c_str());
      updateSta = true;
    }
  }
  if( request->hasParam("wifi_host", true) ) {
    const String& value = request->getParam("wifi_host", true)->value();

    if( ( value != instance->config.getHostName() ) && ( value != "" ) )
    {
      instance->config.setHostName(value.c_str());
      instance->webEvent(EVNT_CONFIG_SET_HOSTNAME, "Hostname changed to %s\n", value.c_str());
      updateHost = configChanged = true;
    }
  }
  if( request->hasParam("wifi_op_mode", true) ) {
    const String& value = request->getParam("wifi_op_mode", true)->value();

    int opMode = value.toInt();
      
    if( instance->config.getWifiOpMode() != opMode )
    {
      instance->config.setWifiOpMode( opMode );
      instance->webEvent(EVNT_CONFIG_SET_OP_MODE, "Wifi opmode changed to %d\n", opMode);
      updateOp = configChanged = true;
    }
  }
  if( request->hasParam("wifi_channel", true) ) {
    const String& value = request->getParam("wifi_channel", true)->value();

    int channel = value.toInt();
      
    if( wifi_get_channel() != channel )
    {
      wifi_set_channel( channel );
      instance->webEvent(EVNT_CONFIG_SET_CHANNEL, "Wifi channel changed to %d\n", channel);
      reset = true;
    }
  }
  if( request->hasParam("wifi_ap_channel", true) ) {
    const String& value = request->getParam("wifi_ap_channel", true)->value();
    int channel = value.toInt();
      
    if( apcfg.channel != channel )
    {
      apcfg.channel = channel;
      instance->webEvent(EVNT_CONFIG_SET_CHANNEL, "Wifi ap channel changed to %d\n", channel);
      updateAp = true;
    }
  }

  if( updateAp )
    wifi_softap_set_config(&apcfg);
  if( updateSta )
    wifi_station_set_config(&cfg);
  if( updateHost )
    wifi_station_set_hostname(instance->config.getHostName());
    
  if( updateAp || updateSta || updateOp )
  {
    if( instance->config.getWifiOpMode() != wifi_get_opmode() )
      wifi_set_opmode(instance->config.getWifiOpMode());
  }

  if( configChanged )
    instance->webEvent(EVNT_CONFIG_CHANGED, "Web configuration changed\n");

  if( reset || updateAp || updateSta || updateOp )
    instance->resetServer();
}

void WebServerCommon::resetServer()
{
  ESP.reset();
}

void WebServerCommon::startServer()
{
  on("/settings.json", HTTP_GET, handleSettingsJsonGet);
  on("/settings.json", HTTP_POST, handleSettingsJsonPost);

  on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  begin();
}

String WebServerCommon::createJSONParamResponse()
{
  return "{\"list\":[";
}

void WebServerCommon::addJSONParam(String &string, const char * name, const String & value, const char * type)
{
  addJSONParam(string, name, value.c_str(), type);
}

void WebServerCommon::addJSONParam(String &string, const char * name, const char * value, const char * type)
{
  if( string.endsWith("]") )
    string += ",";

  string += "[\"";
  string += name;
  string += "\",\"";
  string += value;
  string += "\",\"";
  string += type;
  string += "\"]";
}

void WebServerCommon::addJSONParam(String &string, const char * name, int value, const char * type)
{
  if( string.endsWith("]") )
    string += ",";

  string += "[\"";
  string += name;
  string += "\",";
  string += value;
  string += ",\"";
  string += type;
  string += "\"]";
}

void WebServerCommon::addJSONParam(String &string, const char * name, float value, const char * type)
{
  if( string.endsWith("]") )
    string += ",";

  string += "[\"";
  string += name;
  string += "\",";
  string += value;
  string += ",\"";
  string += type;
  string += "\"]";
}

void WebServerCommon::addJSONParam(String &string, const char * name, const char * type)
{
  if( string.endsWith("]") )
    string += ",";

  string += "[\"";
  string += name;
  string += "\",null,\"";
  string += type;
  string += "\"]";
}

void WebServerCommon::finishJSONParamResponse(String &string)
{
  string += "]}";
}


void WebServerCommon::replyJSON(AsyncWebServerRequest *request, const String &string)
{
  AsyncWebServerResponse * response = request->beginResponse(200, "text/json", string);
  response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate" );
  request->send(response);
}

void WebServerCommon::configureNTP()
{
  char * sntpServer = config.getSNTPServer();
  int zone = config.getSNTPTimezoneOffset();
  configTime(zone * 3600, 0, sntpServer);
}

String WebServerCommon::getTimeString()
{
  time_t tim = time(0);
  struct tm *  tval = localtime(&tim);

  String t = "";
  if( tval->tm_hour < 10 )
    t += "0";
  t += tval->tm_hour;
  t += ":";
  if( tval->tm_min < 10 )
    t += "0";
  t += tval->tm_min;
  t += ":";
  if( tval->tm_sec < 10 )
    t += "0";
  t += tval->tm_sec;
  
  return t;
}

String WebServerCommon::getDateString()
{
  time_t tim = time(0);
  struct tm * tval = localtime(&tim);

  String t = "";
  t += tval->tm_year + 1900;
  t += "-";
  if( tval->tm_mon < 9 )
    t += "0";
  t += tval->tm_mon+1;
  t += "-";
  if( tval->tm_mday < 10 )
    t += "0";
  t += tval->tm_mday;
  
  return t;
}

String WebServerCommon::getDayString()
{
  time_t tim = time(0);
  struct tm *  tval = localtime(&tim);

  const char *day = "";
  switch(tval->tm_wday)
  {
    case 0:
      day = "Vasárnap";
      break;
    case 1:
      day = "Hétfő";
      break;
    case 2:
      day = "Kedd";
      break;
    case 3:
      day = "Szerda";
      break;
    case 4:
      day = "Csütörtök";
      break;
    case 5:
      day = "Péntek";
      break;
    case 6:
      day = "Szombat";
      break;
  }

  return day;
}

void WebServerCommon::handleNTPSettingsJsonGet(AsyncWebServerRequest *request)
{
  String args = createJSONParamResponse();
  addJSONParam(args, "sntp_host", instance->config.getSNTPServer(), "value");
  addJSONParam(args, "sntp_offset", instance->config.getSNTPTimezoneOffset(), "value");
  finishJSONParamResponse(args);
  replyJSON(request, args);
}

void WebServerCommon::handleNTPDateSettingsJsonGet(AsyncWebServerRequest *request)
{
  String args = createJSONParamResponse();
  addJSONParam(args, "sntp_time", instance->getTimeString(), "innerHTML");
  addJSONParam(args, "sntp_date", instance->getDateString(), "innerHTML");
  addJSONParam(args, "sntp_day", instance->getDayString(), "innerHTML");
  finishJSONParamResponse(args);
  replyJSON(request, args);
}

void WebServerCommon::handleNTPSettingsJsonPost(AsyncWebServerRequest *request)
{
  bool configChanged = false;

  if( request->hasParam("redirect", true) ) {
      request->redirect(request->getParam("redirect", true)->value());
  }

  if( request->hasParam("sntp_host", true) ) {
    const String& value = request->getParam("sntp_host", true)->value();

    if( value != instance->config.getSNTPServer() )
    {
      instance->config.setSNTPServer(value.c_str());
      instance->webEvent(EVNT_CONFIG_SNTP_SET_SERVER, "SNTP server changed to %s\n", value.c_str());
      configChanged = true;
    }
  }

  if( request->hasParam("sntp_offset", true) ) {
    const String& value = request->getParam("sntp_offset", true)->value();

    int offset = value.toInt();
      
    if( offset != instance->config.getSNTPTimezoneOffset() )
    {
      instance->config.setSNTPTimezoneOffset(offset);
      instance->webEvent(EVNT_CONFIG_SNTP_SET_OFFSET, "SNTP offset changed to %d\n", offset);
      configChanged = true;
    }
  }

  if( configChanged ) {
    instance->webEvent(EVNT_CONFIG_CHANGED, "Web configuration changed\n");
    instance->configureNTP();
  }
}

void WebServerCommon::enableNTP()
{
  on("/settings.ntp.json", HTTP_GET, handleNTPSettingsJsonGet);
  on("/settings.ntp.date.json", HTTP_GET, handleNTPDateSettingsJsonGet);
  on("/settings.ntp.json", HTTP_POST, handleNTPSettingsJsonPost);
  configureNTP();
}

const char * WebServerCommon::getContentType(const String& path) {
  if (path.endsWith(".html")) return "text/html";
  else if (path.endsWith(".htm")) return "text/html";
  else if (path.endsWith(".css")) return "text/css";
  else if (path.endsWith(".json")) return "text/json";
  else if (path.endsWith(".js")) return "application/javascript";
  else if (path.endsWith(".png")) return "image/png";
  else if (path.endsWith(".gif")) return "image/gif";
  else if (path.endsWith(".jpg")) return "image/jpeg";
  else if (path.endsWith(".ico")) return "image/x-icon";
  else if (path.endsWith(".svg")) return "image/svg+xml";
  else if (path.endsWith(".eot")) return "font/eot";
  else if (path.endsWith(".woff")) return "font/woff";
  else if (path.endsWith(".woff2")) return "font/woff2";
  else if (path.endsWith(".ttf")) return "font/ttf";
  else if (path.endsWith(".xml")) return "text/xml";
  else if (path.endsWith(".pdf")) return "application/pdf";
  else if (path.endsWith(".zip")) return "application/zip";
  else if(path.endsWith(".gz")) return "application/x-gzip";
  else return "text/plain";
}


void WebServerCommon::loop() {
  ArduinoOTA.handle();
}

