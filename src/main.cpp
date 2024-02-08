#include <WiFiManager.h>
#include <time.h>
#include <stdio.h>

#define USEOTA
// enable OTA
#ifdef USEOTA
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#endif


#include "LittleFS.h"
//#include <DNSServer.h>
//#include <ESP8266WebServer.h>
//#include <NTPClient.h>

//#include <Arduino.h>
#include "Button2.h"

// include MDNS
#ifdef ESP8266
//#include <ESP8266mDNS.h>
#endif

// #include "data.h"
// #include "espteleinfo.h"
#include "display.h"
// #include "webserver.h"
// #include <version.h>

#define PIN_OPTO 3
#define PIN_BUTTON 1
#define CONFIG_FILE "/config.dat"
#define RESET_CONFIRM_DELAY 10000
#define SCREEN_OFF_MESSAGE_DELAY 5000
#define SCREENSAVER_DELAY 60000
#define AP_NAME "TeleInfoKit"
#define AP_PWD "givememydata"

#define REFRESH_DELAY 1000

// The structure that stores configuration
typedef struct
{
  char mqtt_server[40];
  char mqtt_port[6];
  char mqtt_server_username[32];
  char mqtt_server_password[32];
  char http_username[32];
  char http_password[32];
  char period_data_power[10];
  char period_data_index[10];
} ConfStruct;

ConfStruct config;

// screen refresh delay
unsigned long refreshTime = 0;
unsigned long mtime = 0;

// all screens available
enum modes
{
  GRAPH,
  DATA1,
  DATA2,
  DATA3,
  NETWORK,
  RESET,
  OFF
};

uint8_t mode = 0;

// network configuration reset states
enum reset_states
{
  IDLE,
  RST_PAGE,
  RST_REQ,
  RST_ACK
};

uint8_t reset = IDLE;



// timestamp for reset request auto-cancellation
unsigned long resetTs = 0;

// flag for saving network configuration
bool shouldSaveConfig = false;

// timestamp for message before screen off
unsigned long offTs = 0;
bool screensaver = false;

bool reset_possible = true;
bool reset_pending = false;

Data *data;
Display *d;
// ESPTeleInfo ti = ESPTeleInfo();
// WebServer *web;
Button2 button = Button2(PIN_BUTTON);


void getTime() {
  int tz           = +1;
  int dst          = 0;
  time_t now       = time(nullptr);
  unsigned timeout = 5000; // try for timeout
  unsigned start   = millis();
  configTime(tz * 3600, dst * 3600, "pool.ntp.org", "time.nist.gov");
  d->log("Waiting for NTP time sync: ");
  while (now < 8 * 3600 * 2 ) { // what is this ?
    delay(100);
    // Serial.print(".");
    now = time(nullptr);
    if((millis() - start) > timeout){
      d->log("[ERROR] Failed to get NTP time.");
      return;
    }
  }
  
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  d->log("Current time: ");
  d->log(asctime(&timeinfo));
}

// #REGION WifiManager ==================================
WiFiManager wm;

bool TEST_NET        = true; // do a network test after connect, (gets ntp time)
bool ALLOWONDEMAND   = true; // enable on demand
bool WMISBLOCKING    = true; // use blocking or non blocking mode, non global params wont work in non blocking

// Network connection has been done through captive portal of hotspot
void saveConfigCallback()
{
  d->log("Configuration sauvée", 500);
  shouldSaveConfig = true;
}

//gets called when WiFiManager enters configuration mode
void configModeCallback(WiFiManager *myWiFiManager)
{
  d->log("Hotspot Wifi: " + myWiFiManager->getConfigPortalSSID() + "\n" + WiFi.softAPIP().toString());
}

void saveParamCallback(){
  d->log("[CALLBACK] saveParamCallback fired", 500);
  // wm.stopConfigPortal();
}

void handleRoute(){
  d->log("[HTTP] handle custom route", 500);
  wm.server->send(200, "text/plain", "hello from user code");
}

void bindServerCallback(){
  wm.server->on("/custom",handleRoute);
}

void handlePreOtaUpdateCallback(){
  Update.onProgress([](unsigned int progress, unsigned int total) {
        d->log("OTA Progress: %u%%\r", (progress / (total / 100)));
  });
}

// /#REGION WifiManager ================================== 




// WiFiEventHandler disconnectedEventHandler;

// WiFiUDP ntpUDP;
// NTPClient timeClient(ntpUDP);

// network configuration variables
char mqtt_server[40];
char mqtt_port[6] = "1883";
char mqtt_server_username[32];
char mqtt_server_password[32];
char http_username[32];
char http_password[32];
char period_data_power[10];
char period_data_index[10];
char UNIQUE_ID [18];

// // Retreives configuration from filesystem
// void readConfig()
// {

//   if (LittleFS.begin())
//   {
//     if (LittleFS.exists(CONFIG_FILE))
//     {
//       //file exists, reading and loading
//       d->logPercent("Lecture configuration", 10);

//       File configFile = LittleFS.open(CONFIG_FILE, "r");
//       if (configFile)
//       {
//         configFile.read((byte *)&config, sizeof(config));

//         strcpy(mqtt_server, config.mqtt_server);
//         strcpy(mqtt_port, config.mqtt_port);
//         strcpy(mqtt_server_username, config.mqtt_server_username);
//         strcpy(mqtt_server_password, config.mqtt_server_password);
//         strcpy(http_username, config.http_username);
//         strcpy(http_password, config.http_password);
//         strcpy(period_data_index, config.period_data_index);
//         strcpy(period_data_power, config.period_data_power);
//         configFile.close();

//         d->logPercent("Configuration chargée", 15);
//       }
//       else
//       {
//         {
//           d->log("Aucun fichier de configuration");
//         }
//       }
//     }
//   }
//   else
//   {
//     d->log("Erreur montage filesystem");
//   }
// }

// Handles clicks on button
void handlerBtn(Button2 &btn)
{
  switch (btn.getType())
  {
  case single_click:
    
    // reset management at startup
    if(reset_possible){
      reset_pending = true;
    }

    if (screensaver == false)
    {
      mode = (mode + 1) % 7; // cycle through 7 screens
    }
    resetTs = 0;
    offTs = millis();
    screensaver = false;
    break;
  case double_click:
    mode = GRAPH;
    resetTs = 0;
    offTs = millis();
    screensaver = false;
    break;
  case triple_click:
    break;
  case long_click:
    // reset state machine mgmt
    if (reset == RST_PAGE)
    {
      reset = RST_REQ;
      // display reset confirmation
      d->log("Appui long pour confirmer\nAppui court pour annuler", 10);
      resetTs = millis();
    }
    else if (reset == RST_REQ)
    {
      reset = RST_ACK;
      // display reset requested
      d->log("Réinitialisation en cours");
      // reset
      wm.resetSettings();
      // ESP.eraseConfig();
      LittleFS.remove(CONFIG_FILE);
      // display restart
      d->log("Redémarrage", 1000);
      // restart
      //ESP.reset();
      wm.reboot();
      delay(200);
    }
    break;
  }
}

void setup()
{
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP  
  // //-pinMode(PIN_OPTO, INPUT_PULLUP);
  data = new Data();
  d = new Display();
  data->init();
  // ti.init();
  d->init(data);
  // web = new WebServer();

  snprintf(UNIQUE_ID, 18, "teleinfokit-%06X", ESP.getChipId());

  d->displayStartup(String(VERSION));
  d->logPercent("Démarrage", 5);

  unsigned long reset_start = millis();

  while(millis() - reset_start < 1000)
  {
    // if button is pressed, reset management
    if(!digitalRead(PIN_BUTTON)){   // no use of click handler because not called yet in a loop
      d->displayReset();
      reset = RST_PAGE;
      unsigned long now = millis();
      while(millis() - now < RESET_CONFIRM_DELAY)
      {
        button.loop();
        delay(10);
      }
      delay(10);
    }
  }

// #REGION WifiManager ==================================
  WiFiManagerParameter custom_html("<p style=\"color:pink;font-weight:Bold;\">This Is Custom HTML</p>"); // only custom html
  WiFiManagerParameter custom_mqtt_server("server", "Serveur MQTT", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "Port MQTT", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_username("username", "MQTT login", mqtt_server_username, 32);
  WiFiManagerParameter custom_mqtt_password("password", "MQTT mot de passe", mqtt_server_password, 32, "type=\"password\"");
  WiFiManagerParameter custom_http_username("http_username", "HTTP login", http_username, 32);
  WiFiManagerParameter custom_http_password("http_password", "HTTP mot de passe", http_password, 32, "type=\"password\"");
  WiFiManagerParameter custom_period_data_power("period_data_power", "Fréquence envoi puissance (secondes)", period_data_power, 10);
  WiFiManagerParameter custom_period_data_index("period_data_index", "Fréquence envoi index (secondes)", period_data_index, 10);

  const char _customHtml_checkbox[] = "type=\"checkbox\""; 
  WiFiManagerParameter custom_checkbox("mode_tic_std", "Mode TIC Standard", "T", 2, _customHtml_checkbox, WFM_LABEL_AFTER);

  wm.setAPCallback(configModeCallback);
  wm.setWebServerCallback(bindServerCallback);
  wm.setSaveConfigCallback(saveConfigCallback);
  wm.setSaveParamsCallback(saveParamCallback);
  wm.setPreOtaUpdateCallback(handlePreOtaUpdateCallback);
  
  wm.addParameter(&custom_html);
  wm.addParameter(&custom_checkbox);
  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_mqtt_username);
  wm.addParameter(&custom_mqtt_password);
  wm.addParameter(&custom_http_username);
  wm.addParameter(&custom_http_password);
  wm.addParameter(&custom_period_data_power);
  wm.addParameter(&custom_period_data_index);

  //custom_mqtt_port.setValue("test",4);  // a creuser pour les valeurs par defaut

  std::vector<const char *> menu = {"wifi","wifinoscan","info","param","custom","close","sep","erase","update","restart","exit"};
  //wm.setMenu(menu); // custom menu, pass vector
  wm.setParamsPage(true);
  // set Hostname

  wm.setHostname(UNIQUE_ID);

  if (!WMISBLOCKING)
  {
    wm.setConfigPortalBlocking(false);
  }

  // show static ip fields
  // wm.setShowStaticFields(true);

  // This is sometimes necessary, it is still unknown when and why this is needed but it may solve some race condition or bug in esp SDK/lib
  wm.setCleanConnect(true); // disconnect before connect, clean connect
  
  wm.setBreakAfterConfig(true); // needed to use saveWifiCallback

  // to preload autoconnect with credentials     A TESTER
  // wm.preloadWiFi("ssid","password");

  d->logPercent("Connexion au réseau wifi.", 25);
  d->logPercent("Connexion au réseau wifi..", 30);
  d->logPercent("Connexion au réseau wifi...", 35);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //and goes into a blocking loop awaiting configuration
  wm.setConnectTimeout(45);
  if (!wm.autoConnect(AP_NAME, AP_PWD))
  {
    d->log("Connexion impossible\n Reset...");
    delay(1000);
    //reset and try again
    ESP.reset();
    delay(1000);
  }
  d->logPercent("Connecté à " + String(WiFi.SSID()), 40);

// /#REGION WifiManager ==================================

  // ================ OTA ================
  #ifdef USEOTA  
  ArduinoOTA.setHostname(UNIQUE_ID);
  ArduinoOTA.setPassword("admin4tele9Info");
  d->logPercent("Démarrage OTA", 60);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
    {
      type = "sketch";
    }
    else
    { // U_SPIFFS
      type = "filesystem";
    }

    d->log("Demarrage MAJ " + type);
  });
  ArduinoOTA.onEnd([]() {
    d->log("Mise à jour complète");
    delay(500);
    d->log("Redémarrage en cours\nVeuillez patienter");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    uint8_t percent = (progress / (total / 100));
    d->logPercent("Mise à jour en cours", percent);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    d->log("OTA error " + (String) error);
    if (error == OTA_AUTH_ERROR)
    {
      d->log("OTA Auth Failed");
    }
    else if (error == OTA_BEGIN_ERROR)
    {
      d->log("OTA Begin Failed");
    }
    else if (error == OTA_CONNECT_ERROR)
    {
      d->log("OTA Connect Failed");
    }
    else if (error == OTA_RECEIVE_ERROR)
    {
      d->log("OTA Receive Failed");
    }
    else if (error == OTA_END_ERROR)
    {
      d->log("OTA End Failed");
    }
  });
  ArduinoOTA.begin();
  #endif


  //WiFi.mode(WIFI_STA);

  // disconnectedEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event)
  // {
  //   d->log("Perte connexion Wifi\n Reset...");
  //   delay(1000);
  //   //reset and try again
  //   ESP.reset();
  //   delay(1000);
  // });

  pinMode(PIN_BUTTON, INPUT_PULLUP);

  button.setClickHandler(handlerBtn);
  button.setDoubleClickHandler(handlerBtn);
  button.setLongClickHandler(handlerBtn);



  //readConfig();

  // uint16_t port = 1883;
  // if (config.mqtt_port[0] != '\0')
  // {
  //   port = atoi(config.mqtt_port);
  //   delay(1000);
  // }

  // ti.initMqtt(config.mqtt_server, port, config.mqtt_server_username, config.mqtt_server_password, atoi(config.period_data_power), atoi(config.period_data_index));


  

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode

  // std::vector<const char *> menu = {"wifi","info","param","sep","restart","exit"};
  // wifiManager.setMenu(menu);

  // // set dark theme
  // wifiManager.setClass("invert");

  // //-WiFi.hostname("TeleInfoKit_" + String(ESP.getChipId()));
  // WiFi.hostname(UNIQUE_ID);


  // strcpy(mqtt_server, custom_mqtt_server.getValue());
  // strcpy(mqtt_port, custom_mqtt_port.getValue());
  // strcpy(mqtt_server_username, custom_mqtt_username.getValue());
  // strcpy(mqtt_server_password, custom_mqtt_password.getValue());
  // strcpy(http_username, custom_http_username.getValue());
  // strcpy(http_password, custom_http_password.getValue());
  // strcpy(period_data_index, custom_period_data_index.getValue());
  // strcpy(period_data_power, custom_period_data_power.getValue());

  // //save the custom parameters to FS
  // if (shouldSaveConfig)
  // {
  //   d->logPercent("Sauvegarde configuration", 45);

  //   File configFile = LittleFS.open(CONFIG_FILE, "w");
  //   if (!configFile)
  //   {
  //     d->log("Erreur écriture config");
  //   }
  //   else
  //   {
  //     strcpy(config.mqtt_server, custom_mqtt_server.getValue());
  //     strcpy(config.mqtt_port, custom_mqtt_port.getValue());
  //     strcpy(config.mqtt_server_username, custom_mqtt_username.getValue());
  //     strcpy(config.mqtt_server_password, custom_mqtt_password.getValue());
  //     strcpy(config.http_username, custom_http_username.getValue());
  //     strcpy(config.http_password, custom_http_password.getValue());
  //     strcpy(config.period_data_index, custom_period_data_index.getValue());
  //     strcpy(config.period_data_power, custom_period_data_power.getValue());
  //     configFile.write((byte *)&config, sizeof(config));
  //     d->logPercent("Configuration sauvée", 50);
  //     delay(250);
  //   }

  //   ti.initMqtt(config.mqtt_server, port, config.mqtt_server_username, config.mqtt_server_password, atoi(config.period_data_power), atoi(config.period_data_index));
  //   configFile.close();
  //   //end save
  // }



  // timeClient.begin();
  // timeClient.update();
  // d->logPercent("Connexion NTP", 75);
  // data->setNtp(&timeClient);

  // web->init(&ti, data, config.mqtt_server, config.mqtt_port, config.mqtt_server_username, config.http_username, config.http_password, atoi(config.period_data_power), atoi(config.period_data_index));

  // d->logPercent("Connexion MQTT", 90);
  // if (!ti.LogStartup())
  // {
  //   d->log("Erreur config MQTT \nRéinitialiser les réglages", 2000);
  // }

    wm.startWebPortal();
  d->logPercent("Démarrage terminé", 300);

  offTs = millis();
  // ti.loop();
}

void loop()
{

  // #ifdef ESP8266
  // MDNS.update();
  // #endif

  #ifdef USEOTA
  ArduinoOTA.handle();
  #endif

  button.loop();
  wm.process();
  // web->loop();

  // for cancelling reset settings requests automatically
  if (resetTs != 0 && (millis() - resetTs > RESET_CONFIRM_DELAY))
  {
    resetTs = 0;
    d->displayReset();
  }

  if (millis() - offTs > SCREENSAVER_DELAY)
  {
    screensaver = true;
    d->displayOff();
  }

  if (millis() - refreshTime > REFRESH_DELAY)
  {
    // if(ti.modeBase)
    // {
    //   data->storeValueBase(ti.base);
    // }
    // else
    // {
    //   data->storeValue(ti.hp, ti.hc);
    // }

    if (!screensaver)
    {
      switch (mode)
      {
      case GRAPH:
        reset = IDLE;
        // d->drawGraph(ti.papp);
        d->drawGraph(0);  // to remove and use commented code
        break;
      case DATA1:
        reset = IDLE;
        // if(ti.modeTriphase){
        //   d->displayData1Triphase(ti.papp, ti.iinst1, ti.iinst2, ti.iinst3);
        // }
        // else{
        //   d->displayData1(ti.papp, ti.iinst);
        // }
        d->displayData1(0, 0);  // to remove and use commented code
        break;
      case DATA2:
        reset = IDLE;
        d->displayData2Base(0);  // to remove and use commented code
        // if(ti.modeBase)
        // {
        //   d->displayData2Base(ti.base);
        // }
        // else
        // {
        //   d->displayData2(ti.hp, ti.hc);
        // }
        break;
      case DATA3:
        reset = IDLE;
        d->displayData3("adc0", 0, 0);  // to remove and use commented code
        // d->displayData3(ti.adc0, ti.isousc, ti.ptec);
        break;
      case NETWORK:
        reset = IDLE;
        d->displayNetwork();
        break;
      case RESET:
        if (reset != RST_REQ && reset != RST_ACK)
        {
          reset = RST_PAGE;
          d->displayReset();
        }
        break;
      case OFF:
        reset = IDLE;
        if (millis() - offTs > SCREEN_OFF_MESSAGE_DELAY)
        {
          d->displayOff();
        }
        else
        {
          d->log("Ecran OFF dans 5s.\nAppui court pour rallumer.", 0);
        }
        break;
      }
    }

    refreshTime = millis();
  }

  // ti.loop();
  // timeClient.update();

    // every 10 seconds
  if(millis()-mtime > 10000 ){
    if(WiFi.status() == WL_CONNECTED){
      getTime();
    }
    else Serial.println("No Wifi");  
    mtime = millis();
  }
}
