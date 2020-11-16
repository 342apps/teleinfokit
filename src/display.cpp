#include "display.h"

// Initialize the OLED display using Arduino Wire:
SSD1306Wire oled(0x3c, 0, 2, GEOMETRY_128_32); // ADDRESS, SDA, SCL, OLEDDISPLAY_GEOMETRY  -  Extra param required for 128x32 displays.

Display::Display()
{
  oled.init();
  oled.flipScreenVertically();
  oled.setFont(ArialMT_Plain_10);
}

void Display::init(Data *d)
{
  data = d;
}

void Display::loop(void)
{
}

void Display::log(String text, int16_t displayTime)
{
  oled.displayOn();
  oled.clear();
  oled.setTextAlignment(TEXT_ALIGN_LEFT);
  oled.setFont(ArialMT_Plain_10);
  oled.drawString(0, 0, text);
  // Serial.println(text);
  oled.display();
  delay(displayTime);
}

void Display::logPercent(String text, int percentage)
{
  oled.displayOn();
  oled.clear();
  oled.setTextAlignment(TEXT_ALIGN_LEFT);
  oled.setFont(ArialMT_Plain_10);
  oled.drawString(0, 0, text);
  oled.drawProgressBar(2, 18, 124, 6, percentage);
  oled.display();
}

void Display::drawGraph(long papp)
{
  oled.displayOn();
  oled.clear();

  data->calculateGraph();

  for (uint8_t i = 0; i < NB_BARS; i++)
  {
    oled.drawHorizontalLine(i * (BAR_WIDTH + 1), HEIGHT - 1, BAR_WIDTH);
    oled.fillRect(i * (BAR_WIDTH + 1), HEIGHT - data->bargraph[i], BAR_WIDTH, data->bargraph[i]);
  }
  oled.drawHorizontalLine(98, HEIGHT - BAR_HEIGHT, 28);
  oled.setTextAlignment(TEXT_ALIGN_RIGHT);
  oled.drawString(WIDTH, HEIGHT - BAR_HEIGHT, String(data->max));
  oled.drawString(WIDTH, HEIGHT - (BAR_HEIGHT / 2), "Wh");
  oled.drawString(128, 0, String(papp) + "VA");
  oled.setTextAlignment(TEXT_ALIGN_LEFT);
  oled.drawString(0, 0, "Historique 24h");
  oled.display();
}

void Display::displayData1(long papp, long iinst)
{
  oled.displayOn();
  oled.clear();
  oled.setTextAlignment(TEXT_ALIGN_LEFT);
  oled.setFont(ArialMT_Plain_10);
  oled.drawString(0, 0, "Puissance / Intensite");
  oled.drawString(0, 10, String(papp) + "VA");
  oled.drawString(0, 20, String(iinst) + "A");
  oled.display();
}

void Display::displayData2(long hp, long hc)
{
  oled.displayOn();
  oled.clear();
  oled.setTextAlignment(TEXT_ALIGN_LEFT);
  oled.setFont(ArialMT_Plain_10);
  oled.drawString(0, 0, "Index compteurs");
  oled.drawString(0, 10, "HC");
  oled.drawString(20, 10, String(hc));
  oled.drawString(0, 20, "HP");
  oled.drawString(20, 20, String(hp));
  oled.display();
}

void Display::displayData3(char *adc0, long isousc, char *ptec)
{
  oled.displayOn();
  oled.clear();
  oled.setTextAlignment(TEXT_ALIGN_LEFT);
  oled.setFont(ArialMT_Plain_10);
  oled.drawString(0, 0, "ID cpt " + String(adc0));
  oled.drawString(0, 10, "Puiss. souscrite : " + String(isousc) + "A");
  oled.drawString(0, 20, "Per. tarif : " + String(ptec));
  oled.display();
}

void Display::displayNetwork()
{
  oled.displayOn();
  oled.clear();
  oled.setTextAlignment(TEXT_ALIGN_LEFT);
  oled.setFont(ArialMT_Plain_10);
  oled.drawString(0, 0, "Wifi " + WiFi.SSID());
  oled.drawString(0, 10, WiFi.localIP().toString());
  oled.drawString(0, 20, WiFi.macAddress());
  oled.display();
}

void Display::displayReset()
{
  oled.displayOn();
  oled.clear();
  oled.setTextAlignment(TEXT_ALIGN_LEFT);
  oled.setFont(ArialMT_Plain_10);
  oled.drawString(0, 0, "Reinitialisation ?");
  oled.drawString(0, 10, "Appui long pour reset...");
  oled.display();
}

void Display::displayOff()
{
  oled.clear();
  oled.displayOff();
}