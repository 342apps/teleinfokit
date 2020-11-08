#include "webserver.h"

ESP8266WebServer server(80);

WebServer::WebServer()
{
}



// Serving Hello world
void getPower() {
    server.send(200, "application/json", "{\"name\": \"Hello getPower\"}");
}

void getIndex() {
    server.send(200, "application/json", "{\"name\": \"Hello getIndex\"}");
}

void getMeterInfo() {
    server.send(200, "application/json", "{\"name\": \"Hello getMeterInfo\"}");
}

// Serving Hello world
void GetSysInfo() {
    String response = "{";
 
    response+= "\"ip\": \""+WiFi.localIP().toString()+"\"";
    response+= ",\"gw\": \""+WiFi.gatewayIP().toString()+"\"";
    response+= ",\"nm\": \""+WiFi.subnetMask().toString()+"\"";
 
    if (server.arg("signalStrength")== "true"){
        response+= ",\"signalStrengh\": \""+String(WiFi.RSSI())+"\"";
    }
 
    if (server.arg("chipInfo")== "true"){
        response+= ",\"chipId\": \""+String(ESP.getChipId())+"\"";
        response+= ",\"flashChipId\": \""+String(ESP.getFlashChipId())+"\"";
        response+= ",\"flashChipSize\": \""+String(ESP.getFlashChipSize())+"\"";
        response+= ",\"flashChipRealSize\": \""+String(ESP.getFlashChipRealSize())+"\"";
    }
    if (server.arg("freeHeap")== "true"){
        response+= ",\"freeHeap\": \""+String(ESP.getFreeHeap())+"\"";
    }
    response+="}";
 
    server.send(200, "application/json", response);
}

// Manage not found URL
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}


 
// Define routing
void restServerRouting() {
    server.on("/", HTTP_GET, []() {
        server.send(200, F("text/html"),
            F("Welcome to the REST Web Server"));
    });
    server.on(F("/power"), HTTP_GET, getPower);
    server.on(F("/index"), HTTP_GET, getIndex);
    server.on(F("/meter"), HTTP_GET, getMeterInfo);
    server.on(F("/info"), HTTP_GET, GetSysInfo);
}



void WebServer::loop()
{
    server.handleClient();
}

void WebServer::init()
{
  // Activate mDNS this is used to be able to connect to the server
  // with local DNS hostmane esp8266.local
//   if (MDNS.begin("esp8266")) {
//     Serial.println("MDNS responder started");
//   }
  // Set server routing
  restServerRouting();
  // Set not found response
  server.onNotFound(handleNotFound);
  // Start server
  server.begin();
  Serial.println("HTTP server started");
}