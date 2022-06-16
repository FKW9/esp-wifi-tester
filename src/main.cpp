#include <Wire.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "config.h"
#include "SSD1306Wire.h"
#include "hte501.h"
#include "FS.h"
#include "SD_MMC.h"

SSD1306Wire display(0x3c, 16, 1);

int8_t rssi = -99;
float rht[] = {0, 0};
unsigned long previous_millis = 0;

bool logging = false;

void setupSDCard();
void setupWiFi();
void logToFile(int8_t rssi, float t, float rh);
void checkWiFiConnection();
void measureRSSI();
void measureTemperatureHumidity();

void setup()
{
  // Serial.begin(115200);
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  setupSDCard();
  setupWiFi();
}

void loop()
{
  checkWiFiConnection();
  unsigned long current_millis = millis();

  if (current_millis - previous_millis >= 1000)
  {
    previous_millis = current_millis;
    measureRSSI();
    measureTemperatureHumidity();

    if (logging)
      logToFile(rssi, rht[1], rht[0]);
  }
}

void setupWiFi()
{

  static uint8_t cnt = 0;

  WiFi.setSleep(false);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_KEY);

  display.clear();
  display.drawStringMaxWidth(0, 0, 128, "Connecting to " + String(WIFI_SSID));
  display.display();

  String waiter = ".";
  while (WiFi.status() != WL_CONNECTED)
  {
    display.drawStringMaxWidth(0, 20, 128, waiter);
    display.display();

    delay(100);
    cnt++;
    waiter += ".";

    if (cnt > 100)
    {
      display.clear();
      display.drawStringMaxWidth(0, 0, 128, "Error connecting to WiFi! Restarting...");
      display.display();
      // Serial.println("Error connecting to WiFi");
      delay(1000);
      ESP.restart();
    }
  }

  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.setPassword(OTA_AUTH);
  ArduinoOTA.begin();

  display.clear();
  display.drawString(0, 0, "Connected!");
  display.drawString(0, 10, "IP: " + WiFi.localIP().toString() + "    CH: " + WiFi.channel());
  display.display();
}

void checkWiFiConnection()
{
  ArduinoOTA.handle();
  if (WiFi.status() != WL_CONNECTED)
  {
    display.clear();
    display.drawStringMaxWidth(0, 0, 128, "Lost connection! Reconnecting...");
    display.display();
    delay(1000);
    WiFi.disconnect();
    yield();
    setupWiFi();
  }
}

void measureRSSI()
{
  rssi = WiFi.RSSI();
  display.setFont(ArialMT_Plain_24);
  display.setColor(BLACK);
  display.fillRect(0, 30, 80, 25);
  display.setColor(WHITE);
  display.drawString(0, 30, String(rssi) + "dBm");
  display.display();
  display.setFont(ArialMT_Plain_10);
}

void measureTemperatureHumidity()
{
  // read sensor, then setup WiFi and send data
  if (getTemperatureHumidity(rht[1], rht[0]))
  {
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.setColor(BLACK);
    display.fillRect(81, 31, 47, 26);
    display.setColor(WHITE);
    display.drawString(128, 32, String(rht[1], 1) + "°C");
    display.drawString(128, 42, String(rht[0], 1) + " %");
    display.display();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
  }
}

void setupSDCard()
{

  display.clear();

  if (!SD_MMC.begin())
  {
    // Serial.println("Card Mount Failed");
    display.drawString(0, 0, "No SD Card found!");
    display.drawString(0, 10, "Data logging disabled.");
    display.display();
    delay(2000);
    return;
  }

  if (SD_MMC.cardType() == CARD_NONE)
  {
    // Serial.println("No SD_MMC card attached");
    display.drawString(0, 0, "No compatible SD Card found!");
    display.drawString(0, 10, "Data logging disabled.");
    display.display();
    delay(2000);
    return;
  }

  display.drawString(0, 0, "Found SD Card!");

  if (!SD_MMC.exists("/data.log"))
  {

    File file = SD_MMC.open("/data.log", FILE_WRITE);

    if (!file)
    {
      display.drawString(0, 10, "Error creating log file!");
      display.drawString(0, 20, "Data logging disabled.");
      display.display();
      delay(2000);
      return;
    }

    if (!file.print("millis;rssi;t;rh\n"))
    {
      file.close();
      display.drawString(0, 10, "Error writing to log file!");
      display.drawString(0, 20, "Data logging disabled.");
      display.display();
      delay(2000);
      return;
    }

    file.close();
  }
  else
  {
    File file = SD_MMC.open("/data.log", FILE_APPEND);
    if (!file)
    {
      file.close();
      display.drawString(0, 10, "Failed to open existing file!");
      display.drawString(0, 20, "Data logging disabled.");
      display.display();
      delay(2000);
      return;
    }
    if (!file.print("\nmillis;rssi;t;rh\n"))
    {
      file.close();
      display.drawString(0, 10, "Failed to write to existing file!");
      display.drawString(0, 20, "Data logging disabled.");
      display.display();
      delay(2000);
      return;
    }
    file.close();
  }

  logging = true;
  display.drawString(0, 10, "Data logging enabled.");
  display.drawString(0, 20, "File: data.log");
  display.display();

  delay(2000);
}

void logToFile(int8_t rssi, float t, float rh)
{
  char buffer[128];
  sprintf(buffer, "%lu;%d;%f;%f\n", millis(), rssi, t, rh);

  File file = SD_MMC.open("/data.log", FILE_APPEND);
  if (!file)
  {
    display.drawString(0, 20, "Failed to open file for appending");
    display.display();
    // Serial.println("Failed to open file for appending");
    file.close();
    return;
  }
  if (!file.print(buffer))
  {
    display.drawString(0, 20, "Append failed");
    display.display();
    // Serial.println("Append failed");
  }
  file.close();
}