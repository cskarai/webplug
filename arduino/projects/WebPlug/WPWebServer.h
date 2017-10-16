#ifndef __WEBPLUGWEBSERVER_H__
#define __WEBPLUGWEBSERVER_H__

#include <WebServerCommon.h>

class WPWebServer : public WebServerCommon
{
private:
  WebServerConfig config;

  static void handleScopeGet(AsyncWebServerRequest *request);
  static void handleRootGet(AsyncWebServerRequest *request);
  static void handleStaticGet(AsyncWebServerRequest *request);
  
public:
  WPWebServer(int port);

  virtual void webEvent(int evntnum, const char *str, ...);
  virtual void startServer();
};

#endif /* __WEBPLUGWEBSERVER_H__  */


