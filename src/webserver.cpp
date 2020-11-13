#include "webserver.h"

ESP8266WebServer server(80);
ESPTeleInfo *teleinfows;
Data *history_data;

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
    String response = "{\"history\": [" ;
    for (uint8_t i = 0; i < NB_BARS-1; i++)
    {
        response += String(history_data->history[i]) + ",";
    }
    response += String(history_data->history[NB_BARS - 1]) + "]}";

    server.send(200, "application/json", response);
}

void getMeterInfo()
{
    server.send(200, "application/json", "{\"adc0\": \"" + String(teleinfows->adc0) + "\", \"isousc\": " + String(teleinfows->isousc) + ", \"ptec\": \"" + String(teleinfows->ptec) + "\"}");
}

void GetSysInfo()
{
    String response = "{";
    response += "\"ip\": \"" + WiFi.localIP().toString() + "\"";
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
    server.on(F("/power"), HTTP_GET, getPower);
    server.on(F("/index"), HTTP_GET, getIndex);
    server.on(F("/history"), HTTP_GET, getHistory);
    server.on(F("/meter"), HTTP_GET, getMeterInfo);
    server.on(F("/info"), HTTP_GET, GetSysInfo);
}

void WebServer::loop()
{
    server.handleClient();
}

void WebServer::init(ESPTeleInfo *ti, Data *d)
{
    teleinfows = ti;
    history_data = d;

    // Set server routing
    restServerRouting();
    // Set not found response
    server.onNotFound(handleNotFound);
    // Start server
    server.begin();
}