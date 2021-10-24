#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include "SSD1306Wire.h"
#include "data.h"
#include "version.h"

#define HEIGHT 32
#define WIDTH 128

class Display
{
public:
    Display();

    void init(Data *d);
    void loop(void);
    void log(String text, int16_t displayTime = 500);
    void logPercent(String text, int percentage);
    void drawGraph(long papp);
    void displayData1(long papp, long iinst);
    void displayData2(long hp, long hc);
    void displayData2Base(long base);
    void displayData3(char *adc0, long isousc, char *ptec);
    void displayNetwork();
    void displayReset();
    void displayOff();
    void displayStartup(String version);

private:
    Data *data;
};

#endif /* DISPLAY_H */