#ifndef ESPTELEINFO_H
#define ESPTELEINFO_H

#define LINE_MAX_COUNT 50
#define DATA_MAX_SIZE 200


#include <ESP8266WiFi.h>
#include <LibTeleinfo.h>
#include <PubSubClient.h>
#include "version.h"

// Linked list structure containing all values received
typedef struct _UnsentValueList UnsentValueList;
struct _UnsentValueList
{
  UnsentValueList *next; // next element
  char  * name;    // LABEL of value name
  char  * value;   // value
};

class ESPTeleInfo
{


public:

    ESPTeleInfo();

    void init(_Mode_e tic_mode);
    void initMqtt(char *server, uint16_t port, char *username, char *password, int period_data);
    void loop(void);

    // les données de consommation
    long iinst;      // HIST: IINST  STD IRMS1
    long papp;        // HIST: PAPP   STD SINSTS
    long index;      // HIST: BASE + HCHC + HCHP   STD EAST
    char adresseCompteur[20];          // HIST: ADCO   STD ADSC
    char strDataTopic[50];
    char strDiscoveryTopic[75];
    long ts_analyzeData;
    char analyzeBuffer[20];


    // _Mode_e tic_mode = TINFO_MODE_STANDARD;
    TInfo tic;
   

    char* _adc0_ = (char *)"ADCO";
    char* _adsc_ = (char *)"ADSC";
    char* _irms1_ = (char *)"IRMS1";
    char* _sinsts_ = (char *)"SINSTS";
    char* _east_ = (char *)"EAST";
    char* _iinst_ = (char *)"IINST";
    char* _papp_ = (char *)"PAPP";
    char* _base_ = (char *)"BASE";
    char* _hchc_ = (char *)"HCHC";
    char* _hchp_ = (char *)"HCHP";

    bool LogStartup();
    // 30 char max !
    void Log(String s);
    void SendAllData();
    void SendAllUnsentData();
    void SendData(char* label, char* value);
    void SetData(char * name, char * val);

    void sendMqttDiscovery();


private:
    _Mode_e ticMode;
    long previousMillis;
    char logBuffer[100];
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
    char bufLabel[10];

    bool sendGenericData();

    void sendMqttDiscoveryIndex(String label, String friendlyName);
    void sendMqttDiscoveryText(String label, String friendlyName);
    void sendMqttDiscoveryForType(String label, String friendlyName, String deviceClass, String unit, String icon);

    char payloadDiscovery[500];

    // detects the communication mode (mono or triphase)
    void getPhaseMode();
    bool connectMqtt();
    void AnalyzeTicForInternalData();

    bool discovery_sent[49];    // flag to specify if mqtt discovery message is sent
    String discoveryDevice;

    UnsentValueList* unsentList = nullptr;
    void freeList(UnsentValueList*& head);
    void addOrReplaceValueInList(UnsentValueList*& head, const char* name, const char* newValue);
    //#define discoveryIndex {"name":"%s","dev_cla":"energy","stat_cla":"total_increasing","unit_of_meas":"kWh","val_tpl":"{{float(value)/1000.0}}","stat_t":"%s/data/%s","uniq_id":"%s-%s","ic":"mdi:counter","dev":{"ids":"%s","name":"TeleInfoKit","sw":"VERSION","mdl":"TeleInfoKit v4"}}
/*
======= Indexes STD
00 EAST     
01 EASF01
02 EASF02
03 EASF03
04 EASF04
05 EASF05
06 EASF06
07 EASF07
08 EASF08
09 EASF09
10 EASF10
11 EASD01
12 EASD02
13 EASD03
14 EASD04

======= Indexes HIST
15 BASE
16 HCHC
17 HCHP
18 EJPHN
19 EJPHPM
20 BBRHCJB
21 BBRHPJB
22 BBRHCJW
23 BBRHPJW
24 BBRHCJR
25 BBRHPJR

======= Power STD
26 SINSTS (STD)
27 SINSTS1 (STD)
28 SINSTS2 (STD)
29 SINSTS3 (STD)

======= Power HIST
30 PAPP (HIST)

======= Intensity STD
31 IRMS1 (STD)
32 IRMS2 (STD)
33 IRMS3 (STD)
34 IINST (HIST)

======= VOLTAGE STD
35 URMS1 (STD)
36 URMS2 (STD)
37 URMS3 (STD)

======= Text STD

38 ADSC (Adresse compteur)
39 NGTF (Nom calendrier tarif)
40 LTARF (Libelle tarif en cours)
41 NTARF (numero index tarif en cours)
42 NJOURF+1
43 MSG1
44 RELAIS

======= Text HIST
45 ADC0 (Adresse compteur)
46 OPTARIF
47 PTEC
48 DEMAIN

*/
};

#endif /* ESPTELEINFO_H */