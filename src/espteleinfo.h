#ifndef ESPTELEINFO_H
#define ESPTELEINFO_H

#define LINE_MAX_COUNT 50
#define DATA_MAX_SIZE 200


#include <ESP8266WiFi.h>
#include <LibTeleinfo.h>
#include <PubSubClient.h>
#include "version.h"

class ESPTeleInfo
{


public:

    ESPTeleInfo();

    void init(_Mode_e tic_mode);
    void initMqtt(char *server, uint16_t port, char *username, char *password, int period_data);
    void loop(void);

    // les données de consommation
    long iinst;      // HIST: IINST  STD IRMS1
    long papp;        // HIST: PAPP   STD SINTS
    long index;      // HIST: BASE + HCHC + HCHP   STD EAST
    char compteur[20];          // HIST: ADCO   STD ADSC
    char strDataTopic[50];

    char values_old[LINE_MAX_COUNT][DATA_MAX_SIZE+1]; //+1 for '\0' ending

    // _Mode_e tic_mode = TINFO_MODE_STANDARD;
    TInfo tic;
   

    char* adc0 = (char *)"ADCO";
    char* adsc = (char *)"ADSC";

    bool LogStartup();
    // 30 char max !
    void Log(String s);
    void SendAllData();
    void SendData(char* label, char* value);
    void SetData(char * name, char * val);



private:
    _Mode_e ticMode;
    long previousMillis;
    bool staticInfoSsent;
    char buffer[30];
    char mqtt_user[32];
    char mqtt_pwd[32];

    // to store 3 for hist mode (BASE, HP, HC) 
    long indexes[3];

    unsigned int delay_generic;
    bool sendGeneric;

    // timestamp for the last power data send
    unsigned long ts_power;
    // timestamp for the last index data send
    unsigned long ts_index;
    // timestamp for the last generic data send
    unsigned long ts_generic;

    char CHIP_ID[7] = {0};
    char UNIQUE_ID [30];

    bool sendGenericData();

    // detects the communication mode (mono or triphase)
    void getPhaseMode();
    bool connectMqtt();
};

#endif /* ESPTELEINFO_H */