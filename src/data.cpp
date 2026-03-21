#include "data.h"

#define HOUR_DELAY (60 * 60 * 1000)

Data::Data()
{
}

void Data::init()
{
    hourTimestamp = 0;
    previousHour = -1;
    // garde fou au demarrage
    lastBase = -1;
    historyReady = false;
    maxGraph = 1; // not 0 because used to divide
    newHour = true;
    firstIndex_base = 0;
    bargraph_float = 0.0;
    maxPower = 0;


    for (uint8_t i = 0; i < NB_BARS; i++)
    {
        history_base[i] = 0;
        bargraph[i] = 0;
    }
}

void Data::setNtp()
{
    now = time(nullptr);
    localtime_r(&now, &timeinfo); // update the structure tm with the current time
    startupTime = now;            // epoch time
    previousHour = timeinfo.tm_hour;
}

void Data::calculateGraph()
{
    calculateMax();

    for (uint8_t i = 0; i < NB_BARS; i++)
    {
        // convert to float to avoid precision loss (result in values = 0)
        bargraph_float = ((float)(history_base[i]) / (float)maxGraph) * (float)BAR_HEIGHT;
        // invert positions to ease the graph generation
        bargraph[NB_BARS - i - 1] = (int8_t)bargraph_float;
    }
}

void Data::storeValueBase(long base)
{
    now = time(nullptr);
    localtime_r(&now, &timeinfo);

    // Ignore base invalid
    if (base <= 0) return;

    if (previousHour != timeinfo.tm_hour)
    {
        shiftIndex();
        hourTimestamp = millis();
        newHour = true;
        previousHour = timeinfo.tm_hour;
    }

    if (newHour)
    {
        firstIndex_base = base;
        newHour = false;
        historyReady = true;      // ✅ historique OK à partir d’ici
        historyStartTime = now;
        history_base[0] = 0;      // ✅ reset conso heure courante
        lastBase = base;          // ✅ mémorise la base
        return;
    }

    if (!historyReady) return;

    // resync si le compteur recule (valeur instable au boot)
    if (base < lastBase)
    {
        firstIndex_base = base;
        lastBase = base;
        history_base[0] = 0;
        return;
    }

    long delta = base - firstIndex_base;

    // garde-fou anti spike (ex: au boot)
    const long MAX_WH_PER_HOUR = 50000L; // ajuste si besoin
    if (delta < 0 || delta > MAX_WH_PER_HOUR)
    {
        firstIndex_base = base;   // resync
        lastBase = base;
        history_base[0] = 0;
        return;
    }

    history_base[0] = delta;
    lastBase = base;
}

void Data::calculateMax()
{
    maxGraph = 1; // not 0 beause used to divide
    for (uint8_t i = 0; i < NB_BARS; i++)
    {
        if (history_base[i] > maxGraph)
        {
            maxGraph = history_base[i];
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

uint32_t Data::getTotal24h()
{
    if (!historyReady) return 0;

    uint32_t total = 0;
    for (uint8_t i = 0; i < NB_BARS; i++)
    {
        long v = history_base[i];
        if (v > 0) total += (uint32_t)v;
    }
    return total;
}
