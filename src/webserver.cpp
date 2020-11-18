#include "webserver.h"

ESP8266WebServer server(80);
ESPTeleInfo *teleinfows;
Data *history_data;
char *config_mqtt_server;
char *config_mqtt_port;
char *config_mqtt_username;

WebServer::WebServer()
{
}

void getPower()
{
    server.send(200, "application/json", "{\"papp\": " + String(teleinfows->papp) + ", \"iinst\": " + String(teleinfows->iinst) + ", \"ptec\": \"" + String(teleinfows->ptec) + "\"}");
}

void getIndex()
{
    server.send(200, "application/json", "{\"hp\": " + String(teleinfows->hp) + ", \"hc\": " + String(teleinfows->hc) + "}");
}

void getHistory()
{

    String response = "{";
    response += "\"historyStartupTime\": " + String(history_data->historyStartTime) + ",";
    response += "\"history_hp\": [" ;
    for (uint8_t i = 0; i < NB_BARS-1; i++)
    {
        response += String(history_data->history_hp[i]) + ",";
    }
    response += String(history_data->history_hp[NB_BARS - 1]) + "],";
    response += "\"history_hc\": [" ;
    for (uint8_t i = 0; i < NB_BARS-1; i++)
    {
        response += String(history_data->history_hc[i]) + ",";
    }
    response += String(history_data->history_hc[NB_BARS - 1]) + "]}";

    server.send(200, "application/json", response);
}

void getMeterInfo()
{
    server.send(200, "application/json", "{\"adc0\": \"" + String(teleinfows->adc0) + "\", \"isousc\": " + String(teleinfows->isousc) + ", \"ptec\": \"" + String(teleinfows->ptec) + "\"}");
}

void getConfigInfo()
{
    server.send(200, "application/json", "{\"mqttServer\": \"" + String(config_mqtt_server) + "\", \"mqttPort\": \"" + String(config_mqtt_port) + "\", \"mqttUsername\": \"" + String(config_mqtt_username) + "\"}");
}

void getSysInfo()
{
    String response = "{";
    response += "\"version\": \"" + String(VERSION) + "\"";
    response += ",\"buildTime\": \"" + String(BUILD_TIME) + "\"";
    response += ",\"ip\": \"" + WiFi.localIP().toString() + "\"";
    response += ",\"gw\": \"" + WiFi.gatewayIP().toString() + "\"";
    response += ",\"nm\": \"" + WiFi.subnetMask().toString() + "\"";
    response += ",\"ssid\": \"" + WiFi.SSID() + "\"";
    response += ",\"mac\": \"" + WiFi.macAddress() + "\"";
    response += ",\"signalStrengh\": \"" + String(WiFi.RSSI()) + "\"";
    response += ",\"chipId\": \"" + String(ESP.getChipId()) + "\"";
    response += ",\"flashChipId\": \"" + String(ESP.getFlashChipId()) + "\"";
    response += ",\"flashChipSize\": \"" + String(ESP.getFlashChipSize()) + "\"";
    response += ",\"flashChipRealSize\": \"" + String(ESP.getFlashChipRealSize()) + "\"";
    response += ",\"freeHeap\": \"" + String(ESP.getFreeHeap()) + "\"";
    response += ",\"startupTime\": " + String(history_data->startupTime);
    response += "}";

    server.send(200, "application/json", response);
}

// Manage not found URL
void handleNotFound()
{
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++)
    {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
}

// Define routing
void restServerRouting()
{
    server.serveStatic("/", LittleFS, "/index.html");
    server.serveStatic("/chartbulb-160.gif", LittleFS, "/chartbulb-160.gif");
    server.on(F("/power"), HTTP_GET, getPower);
    server.on(F("/index"), HTTP_GET, getIndex);
    server.on(F("/history"), HTTP_GET, getHistory);
    server.on(F("/meter"), HTTP_GET, getMeterInfo);
    server.on(F("/info"), HTTP_GET, getSysInfo);
    server.on(F("/config"), HTTP_GET, getConfigInfo);
}

void WebServer::loop()
{
    server.handleClient();
}

void WebServer::init(ESPTeleInfo *ti, Data *d, char *conf_mqtt_server, char *conf_mqtt_port, char *conf_mqtt_username)
{
    teleinfows = ti;
    history_data = d;
    config_mqtt_port = conf_mqtt_port;
    config_mqtt_server = conf_mqtt_server;
    config_mqtt_username = conf_mqtt_username;

    // Set server routing
    restServerRouting();
    // Set not found response
    server.onNotFound(handleNotFound);
    // Start server
    server.begin();
}