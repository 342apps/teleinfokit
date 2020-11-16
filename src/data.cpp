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
    firstIndex = 0;
    bargraph_float = 0.0;

    for (uint8_t i = 0; i < NB_BARS; i++)
    {
        history[i] = 0;
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
        bargraph_float = ((float)history[i] / (float)max) * (float)BAR_HEIGHT;
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
            firstIndex = hp + hc;
            newHour = false;
        }

        historyStartTime = ntpClient->getEpochTime();
    }
    else
    {
        history[0] = hp + hc - firstIndex;
    }
}

void Data::calculeMax()
{
    max = 1; // not 0 beause used to divide
    for (uint8_t i = 0; i < NB_BARS; i++)
    {
        if (history[i] > max)
        {
            max = history[i];
        }
    }
}

// Shift indexes in history array to let a new one enter and drop the oldest
void Data::decaleIndex()
{
    for (uint8_t i = NB_BARS - 1; i > 0; i--)
    {
        history[i] = history[i - 1];
    }

    history[0] = 0;
}