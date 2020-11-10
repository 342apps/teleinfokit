#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "LittleFS.h"
#include "espteleinfo.h"

class WebServer
{
public:
    WebServer();

    void init(ESPTeleInfo *ti);
    void loop();

private:
};

#endif /* WEBSERVER_H */