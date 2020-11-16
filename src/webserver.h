#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "LittleFS.h"
#include "espteleinfo.h"
#include "data.h"

class WebServer
{
public:
    WebServer();

    void init(ESPTeleInfo *ti, Data *d, char *conf_mqtt_server, char *conf_mqtt_port, char *conf_mqtt_username);
    void loop();

private:
};

#endif /* WEBSERVER_H */