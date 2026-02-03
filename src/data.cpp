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

void Data::setNtp()
{
    now = time(nullptr);
    localtime_r(&now, &timeinfo); // update the structure tm with the current time
    startupTime = now;
    previousHour = timeinfo.tm_hour;
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

void Data::storeValueBase(long indexTotal)
{
    // Index invalide → on ignore
    if (indexTotal <= 0)
        return;

    now = time(nullptr);
    localtime_r(&now, &timeinfo);

    // Première valeur : on ne calcule RIEN
    if (!hasPreviousIndex)
    {
        previousIndex = indexTotal;
        hasPreviousIndex = true;
        previousHour = timeinfo.tm_hour;

        // reset propre
        for (uint8_t i = 0; i < NB_BARS; i++)
            history_base[i] = 0;

        return;
    }

    // Delta réel depuis la dernière lecture
    long delta = indexTotal - previousIndex;

    // Protection anti-délire
    if (delta < 0 || delta > 50000) // 50 kWh max par pas
    {
        previousIndex = indexTotal;
        return;
    }

    history_base[0] += delta;
    previousIndex = indexTotal;

    // Changement d'heure → on décale
    if (timeinfo.tm_year > 120 && timeinfo.tm_hour != previousHour)
    {
        shiftIndex();
        previousHour = timeinfo.tm_hour;
        history_base[0] = 0;
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