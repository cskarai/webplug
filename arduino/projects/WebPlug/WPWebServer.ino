#include "WebPlug.h"
#include "WPWebServer.h"
#include <xprintf.h>
#include "Attiny.h"
#include "WebPages.h"
#include "Config.h"

#define FLSTR(s) ((const __FlashStringHelper*)(s))

extern "C"{
 #include "user_interface.h"
}

WPWebServer::WPWebServer(int port) : WebServerCommon(::config, port)
{
}

void WPWebServer::webEvent(int evntnum, const char *format, ...) {

  switch(evntnum)
  {
    case EVNT_OTA_HEARTBEAT:
    case EVNT_CONNECTING:
      break;
    default:
      {
        char buff[256];

        va_list args;
        va_start(args, format);
        xvsprintf(buff, format, args);
        va_end(args);

        DBG("%s", buff);
      }
      break;
  }
}

void WPWebServer::handleScopeGet(AsyncWebServerRequest *request)
{
  WebServerCommon * instance = WebServerCommon::getInstance();

  String s = "{";
  s += "\"factor\":" + String(::config.getInterpolationFactor()) + ",";
  s += "\"controlPoints\":" + String(::config.isInterpolationControlPointDraw() ? "true" : "false") + ",";
  s += "\"data\":[";

  s += attiny.getOscilloscopeData();

  s += "]}";

  instance->replyJSON(request, s);
}

void WPWebServer::handleRootGet(AsyncWebServerRequest *request)
{
  request->redirect("/vezerles.html");
}

void WPWebServer::handleStaticGet(AsyncWebServerRequest *request)
{
  for( int i=0; i < sizeof(webPages) / sizeof(WEBCONTENT); i++) {
    struct WEBCONTENT * content = webPages + i;
    String url = FLSTR(content->url);

    if( request->url() == url ) {
      AsyncWebServerResponse * response = request->beginResponse_P(200, WebServerCommon::getContentType(url), content->content, content->size); 
      if( content->zipped )
        response->addHeader("Content-Encoding", "gzip");
      response->addHeader("Cache-Control", "max-age=3600, must-revalidate");
      request->send(response);
    }
  }
}

void WPWebServer::startServer()
{
  on("/oscilloscope.json", HTTP_GET, handleScopeGet);
  on("/", HTTP_GET, handleRootGet);
  
  for( int i=0; i < sizeof(webPages) / sizeof(WEBCONTENT); i++) {
    struct WEBCONTENT * content = webPages + i;
    String url = FLSTR(content->url);
    on(url.c_str(), HTTP_GET, handleStaticGet);
  }
  
  enableNTP();
  WebServerCommon::startServer();
}


