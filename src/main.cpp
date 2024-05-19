#define HARDWARE "ESP32"
#include <Arduino.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <SPIFFS.h>
#include <FS.h>

#include "utils.h"
#include "ledMatrixDisplay.h"
#include "MlbTeam.h"
#include "EEPROM_locations.h"
#include "MyServer.h"
#include "segmentDisplay.h"

// assume 7 segment display and each segment has 4 LEDS

// 7 segmemnt display defines
// task defines
#define TEAM_UPDATE_INTERVAL 7500
#define MATRIX_UPDATE_INTERVAL 18
#define DEFAULT_BRIGHTNESS 50
#define POTENTIOMETER_PIN 33
#define MAX_BRIGHTNESS 200
#define RESTART_EVERY_N_HOURS 6

int loopNumber = 0;
bool firstTeamUpdateDone = false;
MlbTeam padres(PADRES_TEAM_ID);
LedMatrixDisplay ledMatrix(LED_MATRIX_PIN);
const String access_point_ssid = "PadresScoreboard";
const String access_point_password = "password";
IPAddress access_point_ip_address;
bool EEPROM_INITIALIZED = false;
int startTime = 0;

void connect_wifi()
{
  Serial.println("Reading ssid and password from EEPROM");
  String ssid = EEPROM.readString(SSID_LOCATION);
  String password = EEPROM.readString(PASSWORD_LOCATION);

  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
  Serial.println("connecting wifi");
  WiFi.begin(ssid, password);
  int i = 0;
  ledText text = {"Connecting WIFI...Access Point Ip Address: " + access_point_ip_address.toString(), 0, 0, 255};
  ledMatrix.showSingleText(text);

  while (WiFi.status() != WL_CONNECTED)
  { // Wait for the Wi-Fi to connect
    delay(500);
    Serial.print(++i);
    Serial.print(' ');
    if (i > 10)
    {
      return;
    }
  }
  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP()); // Send the IP address of the ESP8266 to the computer

  xTaskCreate(serverLoop, "serverLoop", 10000, NULL, 0, NULL);
}

int lastBrightness = 0;
void ledLoop(void *pvParameters)
{
  while (true)
  {
    uint16_t potValue = analogRead(POTENTIOMETER_PIN);
    uint8_t brightness = map(4095 - potValue, 0, 4095, 5, 175);
    if (abs(brightness - lastBrightness) >= 3)
    {
      lastBrightness = brightness;
      setSegmentBrightness(brightness);
      ledMatrix.setBrightness(brightness);
    }
    ledMatrix.loopMatrix();
    showSegmentDisplay();
    delay(MATRIX_UPDATE_INTERVAL);
  }
}

void teamUpdateLoop(void *pvParameters)
{
  while (true)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("Wifi not connected, reconnecting...");
      connect_wifi();
      delay(100);
      continue;
    }
    padres.update();
    clearSegmentDisplay();
    if (padres.isPlaying)
    {
      // show score
      gameInfo game = padres.currentGame;
      gameScore score = padres.score;
      bool padresHome = game.homeTeamId == PADRES_TEAM_ID;
      illuminateNumber(score.homeScore, padresHome ? 0 : 255, padresHome ? 255 : 0, 0, 0);
      illuminateNumber(score.awayScore, padresHome ? 255 : 0, padresHome ? 0 : 255, 0, 2);
      Serial.println("Padres are the home team");
      showSegmentDisplay();

      // SHOW SOME TEXT ON THE ARRAY
      String gameStr = "GAME ON VS " + (padresHome ? game.awayTeam : game.homeTeam) + "!";
      String inningStr = "INNING:" + game.inningState + " of " + game.currentInning;
      String padresRecordStr = "Padres Record: " + String(padres.record.wins) + "-" + String(padres.record.losses);
      ledText *texts = new ledText[3]{{gameStr, 255, 255, 0}, {inningStr, 0, 255, 0}, {padresRecordStr, 0, 0, 255}};
      ledMatrix.showMultiTexts(texts, 3);
    }
    else
    {
      // show record
      winLossRecord record = padres.record;
      illuminateNumber(record.wins, 0, 255, 0, 0);
      illuminateNumber(record.losses, 255, 0, 0, 2);
      showSegmentDisplay();

      // SHOW SOME TEXT ON THE ARRAY
      bool padresHomeNextGame = padres.nextGame.homeTeamId == PADRES_TEAM_ID;
      char buf[1024];
      String nextGameStr = "NEXT GAME VS " + (padresHomeNextGame ? padres.nextGame.awayTeam : padres.nextGame.homeTeam) + " AT " + padres.nextGame.startTime + "!";
      String gameBackStr = "GAMES BACK NLWEST:" + String(padres.gamesBackFromFirst) + " WC:" + String(padres.gamesBackFromWildcard);
      String divisionRankStr = "DIV RANK:" + String(padres.divisionRank) + " WC RANK:" + String(padres.wildCardRank);
      String streakStr = "Streak:" + padres.streakType + " " + padres.streakNumber;
      bool isWinStreak = padres.streakType == "wins";
      ledText *texts = new ledText[4]{{nextGameStr, 255, 255, 0}, {gameBackStr, 255, 0, 0}, {divisionRankStr, 0, 0, 255}, {streakStr, isWinStreak ? 0 : 255, isWinStreak ? 255 : 0, 0}};

      ledMatrix.showMultiTexts(texts, 4);
    }
    delay(TEAM_UPDATE_INTERVAL);
  }
}

///////////////////////// SETUP AND LOOP //////////////////////////
void setup()
{
  Serial.begin(115200);
  // Serial.println("Made it here 2");
  if (!SPIFFS.begin(true))
  {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  EEPROM.begin(1024); // Initialize EEPROM
  EEPROM_INITIALIZED = EEPROM.readString(EEPROM_INITALIZED_LOCATION) == "INITIALIZED";
  if (!EEPROM_INITIALIZED)
  {
    Serial.println("Initializing EEPROM");
    EEPROM.writeString(EEPROM_INITALIZED_LOCATION, "INITIALIZED");
    EEPROM.writeString(SSID_LOCATION, "ssid");
    EEPROM.writeString(PASSWORD_LOCATION, "password");
    EEPROM.write(LAST_SEG_DISPLAY_BRIGHTNESS_LOCATION, DEFAULT_BRIGHTNESS);
    EEPROM.write(LAST_MATRIX_BRIGHTNESS_LOCATION, DEFAULT_BRIGHTNESS);
    EEPROM.commit();
  }

  EEPROM.writeString(SSID_LOCATION, "T_TOWN-2G");
  EEPROM.writeString(PASSWORD_LOCATION, "People46");
  EEPROM.commit();
  uint8_t lastSegBrightness = EEPROM.read(LAST_SEG_DISPLAY_BRIGHTNESS_LOCATION);
  uint8_t lastMatBrightness = EEPROM.read(LAST_MATRIX_BRIGHTNESS_LOCATION);
  Serial.print("Last Segment Display Brightness: ");
  Serial.println(lastSegBrightness);
  Serial.print("Last Matrix Brightness: ");
  Serial.println(lastMatBrightness);
  setupSegmentDisplay(lastSegBrightness);
  ledMatrix.setupMatrix(lastMatBrightness);

  WiFi.softAP(access_point_ssid, access_point_password);
  WiFi.softAPsetHostname("padres_scoreboard");
  access_point_ip_address = WiFi.softAPIP();

  Serial.print("Access point ip address:");
  Serial.println(access_point_ip_address);
  xTaskCreatePinnedToCore(
      teamUpdateLoop,   /* Function to implement the task */
      "teamUpdateLoop", /* Name of the task */
      10000,            /* Stack size in words */
      NULL,             /* Task input parameter */
      0,                /* Priority of the task */
      NULL,             /* Task handle. */
      0);               /* Core where the task should run */

  delay(1000);

  xTaskCreatePinnedToCore(
      ledLoop,   /* Function to implement the task */
      "ledLoop", /* Name of the task */
      10000,     /* Stack size in words */
      NULL,      /* Task input parameter */
      1,         /* Priority of the task */
      NULL,      /* Task handle. */
      1);        /* Core where the task should run */
  startTime = millis();
}

void loop()
{
  delay(1000 * 60);
  // restart every 24 hours
  if (millis() - startTime > 1000 * 60 * 60 * RESTART_EVERY_N_HOURS)
  {
    ESP.restart();
  }
}