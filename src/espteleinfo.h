#ifndef ESPTELEINFO_H
#define ESPTELEINFO_H

#include <TeleInfo.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

class ESPTeleInfo
{
public:
    ESPTeleInfo();

    void init();
    void initMqtt(char *server, uint16_t port, char *username, char *password);
    void loop(void);

    // les données de consommation
    long iinst, iinst_old;
    long papp, papp_old;
    long hc, hc_old;
    long hp, hp_old;
    long imax, imax_old;
    long isousc, isousc_old;
    char adc0[20];
    char ptec[20];
    char ptec_old[20];
    bool LogStartup();
    // 30 char max !
    void Log(String s);

private:
    long previousMillis;
    bool staticInfoSsent;
    char buffer[30];
    char mqtt_user[20];
    char mqtt_pwd[20];

    bool connectMqtt();
};

#endif /* ESPTELEINFO_H */