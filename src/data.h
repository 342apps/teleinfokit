#ifndef DATA_H
#define DATA_H

#include <Arduino.h>
#include <NTPClient.h>

#define NB_BARS 24
#define BAR_WIDTH 3
#define BAR_HEIGHT 20

class Data
{
public:
    Data();

    void init();
    void loop(void);

    // les données de consommation
    long history_hp[NB_BARS];
    long history_hc[NB_BARS];
    long history_base[NB_BARS];
    long max;
    uint8_t bargraph[NB_BARS];
    unsigned long startupTime;
    unsigned long historyStartTime;

    void calculateGraph();
    void storeValue(long hp, long hc);
    void storeValueBase(long base);
    void setNtp(NTPClient *ntp);

private:
    void calculateMax();
    void shiftIndex();
    bool newHour;
    unsigned long hourTimestamp;
    long firstIndex_hp;
    long firstIndex_hc;
    long firstIndex_base;
    float bargraph_float;
    NTPClient *ntpClient;
    int previousHour;
};

#endif /* DATA_H */
