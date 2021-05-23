#ifndef ESPTELEINFO_H
#define ESPTELEINFO_H

#include <TeleInfo.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "version.h"

class ESPTeleInfo
{
public:
    ESPTeleInfo();

    void init();
    void initMqtt(char *server, uint16_t port, char *username, char *password, int period_data_power, int period_data_index);
    void loop(void);

    // les données de consommation
    long iinst, iinst_old;
    long papp, papp_old;
    long hc, hc_old;
    long hp, hp_old;
    long base, base_old;
    long imax, imax_old;
    long isousc, isousc_old;
    char adc0[20];
    char ptec[20];
    char ptec_old[20];

    bool modeBase;

    bool LogStartup();
    // 30 char max !
    void Log(String s);

private:
    long previousMillis;
    bool staticInfoSsent;
    char buffer[30];
    char mqtt_user[20];
    char mqtt_pwd[20];

    unsigned int delay_power;
    unsigned int delay_index;
    bool sendPower;
    bool sendIndex;
    // timestamp for the last power data send
    unsigned long ts_power;
    // timestamp for the last index data send
    unsigned long ts_index;

    char CHIP_ID[7] = {0};

    bool sendPowerData();
    bool sendIndexData();
    bool connectMqtt();
};

#endif /* ESPTELEINFO_H */