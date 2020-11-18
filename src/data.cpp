#include "data.h"

#define HOUR_DELAY (60 * 60 * 1000)

Data::Data()
{
}

void Data::init()
{
    hourTimestamp = 0;
    previousHour = -1;
    max = 1; // not 0 beause used to divide
    newHour = true;
    firstIndex_hp = 0;
    firstIndex_hc = 0;
    bargraph_float = 0.0;

    for (uint8_t i = 0; i < NB_BARS; i++)
    {
        history_hp[i] = 0;
        history_hc[i] = 0;
        bargraph[i] = 0;
    }
}

void Data::setNtp(NTPClient *ntp){
    ntpClient = ntp;
    startupTime = ntpClient->getEpochTime();
    previousHour = ntpClient->getHours();
}

void Data::calculateGraph()
{
    calculeMax();

    for (uint8_t i = 0; i < NB_BARS; i++)
    {
        // convert to float to avoir precision loss (result in values = 0)
        bargraph_float = ((float)(history_hp[i] + history_hc[i]) / (float)max) * (float)BAR_HEIGHT;
        // invert positions to ease the graph generation
        bargraph[NB_BARS - i - 1] = (int8_t)bargraph_float;
    }
}

void Data::storeValue(long hp, long hc)
{
    if ((unsigned long)(millis() - hourTimestamp) >= HOUR_DELAY || previousHour != ntpClient->getHours())
    {
        // each hour
        decaleIndex();
        hourTimestamp = millis();
        newHour = true;
        previousHour = ntpClient->getHours();
    }

    if (newHour)
    {
        if (hp != 0 && hc != 0)
        {
            firstIndex_hp = hp;
            firstIndex_hc = hc;
            newHour = false;
        }

        historyStartTime = ntpClient->getEpochTime();
    }
    else
    {
        history_hp[0] = hp - firstIndex_hp;
        history_hc[0] = hc - firstIndex_hc;
    }
}

void Data::calculeMax()
{
    max = 1; // not 0 beause used to divide
    for (uint8_t i = 0; i < NB_BARS; i++)
    {
        if ((history_hp[i] + history_hc[i]) > max)
        {
            max = history_hp[i] + history_hc[i];
        }
    }
}

// Shift indexes in history array to let a new one enter and drop the oldest
void Data::decaleIndex()
{
    for (uint8_t i = NB_BARS - 1; i > 0; i--)
    {
        history_hp[i] = history_hp[i - 1];
        history_hc[i] = history_hc[i - 1];
    }

    history_hp[0] = 0;
    history_hc[0] = 0;
}