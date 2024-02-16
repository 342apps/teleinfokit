#include "data.h"

#define HOUR_DELAY (60 * 60 * 1000)

Data::Data()
{
}

void Data::init()
{
    hourTimestamp = 0;
    previousHour = -1;
    max = 1; // not 0 because used to divide
    newHour = true;
    firstIndex_base = 0;
    bargraph_float = 0.0;

    for (uint8_t i = 0; i < NB_BARS; i++)
    {
        history_base[i] = 0;
        bargraph[i] = 0;
    }
}

void Data::setNtp(){
    now = time(nullptr);
    localtime_r(&now, &timeinfo); // update the structure tm with the current time
    // strftime (buffer,80,"%a %d %b %Y %H:%M:%S ", &timeinfo);
    startupTime = now;  // epoch time
    previousHour = timeinfo.tm_hour;
    // previousHour = ntpClient->getHours();
}

void Data::calculateGraph()
{
    calculateMax();

    for (uint8_t i = 0; i < NB_BARS; i++)
    {
        // convert to float to avoid precision loss (result in values = 0)
        bargraph_float = ((float)(history_base[i]) / (float)max) * (float)BAR_HEIGHT;
        // invert positions to ease the graph generation
        bargraph[NB_BARS - i - 1] = (int8_t)bargraph_float;
    }
}

void Data::storeValueBase(long base)
{
    localtime_r(&now, &timeinfo); // update the structure tm with the current time
    if (previousHour != timeinfo.tm_hour)
    {
        // each hour
        shiftIndex();
        hourTimestamp = millis();
        newHour = true;
        previousHour = timeinfo.tm_hour;
    }

    if (newHour)
    {
        if (base != 0)
        {
            firstIndex_base = base;
            newHour = false;
        }

        historyStartTime = now;
    }
    else
    {
        history_base[0] = base - firstIndex_base;
    }
}

void Data::calculateMax()
{
    max = 1; // not 0 beause used to divide
    for (uint8_t i = 0; i < NB_BARS; i++)
    {
        if (history_base[i] > max)
        {
            max = history_base[i];
        }
    }
}

// Shift indexes in history array to let a new one enter and drop the oldest
void Data::shiftIndex()
{
    for (uint8_t i = NB_BARS - 1; i > 0; i--)
    {
        history_base[i] = history_base[i - 1];
    }

    history_base[0] = 0;
}