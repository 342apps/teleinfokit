#include "display.h"

// Initialize the OLED display using Arduino Wire:
SSD1306Wire oled(0x3c, 0, 2, GEOMETRY_128_32); // ADDRESS, SDA, SCL, OLEDDISPLAY_GEOMETRY  -  Extra param required for 128x32 displays.

Display::Display()
{
  oled.init();
#if _HW_VER == 1
  oled.flipScreenVertically();
#endif
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
  oled.display();
  delay(displayTime);
}

void Display::displayStartup(String version)
{
  oled.displayOn();
  oled.clear();
  oled.setTextAlignment(TEXT_ALIGN_LEFT);
  oled.setFont(ArialMT_Plain_16);
  oled.drawString(36, 0, "TeleInfoKit");
  oled.setFont(ArialMT_Plain_10);
  oled.drawString(36, 20, version);

  // ## LOGO ##

  oled.drawLine(2, 16, 7, 2);
  oled.drawLine(3, 16, 8, 2);

  oled.drawLine(8, 16, 13, 2);
  oled.drawLine(9, 16, 14, 2);

  oled.drawLine(14, 16, 19, 2);
  oled.drawLine(15, 16, 20, 2);

  oled.drawLine(5, 31, 10, 17);
  oled.drawLine(6, 31, 11, 17);

  oled.drawLine(11, 31, 16, 17);
  oled.drawLine(12, 31, 17, 17);

  oled.drawLine(17, 31, 22, 17);
  oled.drawLine(18, 31, 23, 17);

  oled.drawLine(22, 31, 27, 17);
  oled.drawLine(23, 31, 28, 17);

  oled.drawLine(5, 17, 6, 17);
  oled.drawLine(6, 17, 11, 31);

  oled.drawLine(11, 17, 11, 17);
  oled.drawLine(12, 17, 17, 31);

  oled.display();
  delay(800);
}

void Display::logPercent(String text, int percentage)
{
  oled.displayOn();
  oled.clear();
  oled.setTextAlignment(TEXT_ALIGN_CENTER);
  oled.setFont(ArialMT_Plain_16);
  oled.drawString(64, 0, "TeleInfoKit");
  oled.setFont(ArialMT_Plain_10);
  oled.drawString(64, 13, text);
  oled.drawProgressBar(2, 27, 123, 4, percentage);
  oled.display();
}

void Display::drawGraph(long papp, char mode)
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
  oled.drawRect(0, 1, 9, 11);
  oled.drawString(1, 0, String(mode));
  oled.drawString(12, 0, "Graphe 24h");
  oled.display();
}

void Display::displayData1(long papp, long iinst)
{
  oled.displayOn();
  oled.clear();
  oled.setTextAlignment(TEXT_ALIGN_LEFT);
  oled.setFont(ArialMT_Plain_10);
  oled.drawString(0, 0, "Puissance / Intensité");
  oled.drawString(0, 10, String(papp) + "VA");
  oled.drawString(0, 20, String(iinst) + "A");
  oled.display();
}

void Display::displayData2(long index, char *compteur)
{
  oled.displayOn();
  oled.clear();
  oled.setTextAlignment(TEXT_ALIGN_LEFT);
  oled.setFont(ArialMT_Plain_10);
  oled.drawString(0, 0, "Compteur");
  oled.drawString(0, 10, "Index total");
  oled.setTextAlignment(TEXT_ALIGN_RIGHT);
  oled.drawString(120, 0, compteur);
  oled.drawString(120, 10, String(index));
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

void Display::displayTime()
{
  struct tm timeinfo;
  localtime_r(&now, &timeinfo); // update the structure tm with the current time
  strftime(buffer, 80, "%a %d %b %Y %H:%M:%S ", &timeinfo);
  oled.displayOn();
  oled.clear();
  oled.setTextAlignment(TEXT_ALIGN_LEFT);
  oled.setFont(ArialMT_Plain_10);
  oled.drawString(0, 0, "Date / heure ");
  oled.drawString(0, 10, buffer);
  oled.display();
}

void Display::displayReset(String apKey)
{
  oled.displayOn();
  oled.clear();
  oled.setTextAlignment(TEXT_ALIGN_LEFT);
  oled.setFont(ArialMT_Plain_10);
  oled.drawString(0, 0, "Réinit ?");
  oled.drawString(0, 10, "Appui long pour reset...");
  oled.drawString(0, 20, apKey);
  oled.setTextAlignment(TEXT_ALIGN_RIGHT);
  oled.drawString(128, 0, VERSION);
  oled.display();
}

void Display::displayOff()
{
  oled.clear();
  oled.displayOff();
}

void Display::displayTestTic(String power, String index, char ticMode)
{
  oled.displayOn();
  oled.clear();
  oled.setTextAlignment(TEXT_ALIGN_LEFT);
  oled.setFont(ArialMT_Plain_10);
  oled.drawString(0, 0, "TIC TEST");
  oled.drawRect(118, 1, 9, 11);
  oled.drawString(119, 0, String(ticMode));
  oled.drawString(0, 10, "POWER");
  oled.drawString(0, 20, "INDEX");
  oled.setTextAlignment(TEXT_ALIGN_RIGHT);
  oled.drawString(128, 10, String(power));
  oled.drawString(128, 20, String(index));
  oled.display();
}

void Display::getTime()
{
  now = time(nullptr);
  unsigned timeout = 5000; // try for timeout
  unsigned start = millis();

  configTime(TZ_Europe_Paris, "pool.ntp.org", "time.nist.gov");
  while (now < 8 * 3600 * 2)
  { // what is this ?
    delay(100);
    // Serial.print(".");
    now = time(nullptr);
    if ((millis() - start) > timeout)
    {
      oled.drawString(0, 0, "[ERROR] Failed to get NTP time.");
      return;
    }
  }
}