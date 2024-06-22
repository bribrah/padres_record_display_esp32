#define HARDWARE "ESP32"
#include <Arduino.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <SPIFFS.h>
#include <FS.h>
#include <esp_task_wdt.h>

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
#define MATRIX_UPDATE_INTERVAL 15
#define DEFAULT_BRIGHTNESS 50
#define POTENTIOMETER_PIN 33
#define MAX_BRIGHTNESS 200
#define RESTART_EVERY_N_HOURS 24

int loopNumber = 0;
bool firstTeamUpdateDone = false;
MlbTeam padres(PADRES_TEAM_ID);
LedMatrixDisplay ledMatrix(LED_MATRIX_PIN);
const String access_point_ssid = "PadresScoreboard";
const String access_point_password = "password";
IPAddress access_point_ip_address;
bool EEPROM_INITIALIZED = false;
int startTime = 0;
TaskHandle_t *serverTask;

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

  // restart the server task
  if (serverTask != NULL)
  {
    vTaskDelete(serverTask);
  }
  xTaskCreatePinnedToCore(serverLoop, "serverLoop", 8092, NULL, 0, serverTask, 1);
}

int lastBrightness = 0;
void ledLoop(void *pvParameters)
{
  esp_task_wdt_add(NULL); // Add the current thread to the watchdog monitor
  while (true)
  {
    esp_task_wdt_reset(); // Reset the watchdog timer
    uint16_t potValue = analogRead(POTENTIOMETER_PIN);
    uint8_t brightness = map(4095 - potValue, 0, 4095, 5, MAX_BRIGHTNESS);
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

ledText texts[MAX_TEXTS];
void teamUpdateLoop(void *pvParameters)
{
  esp_task_wdt_add(NULL); // Add current task to watchdog
  while (true)
  {
    esp_task_wdt_reset();
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("Wifi not connected, reconnecting...");
      connect_wifi();
      delay(100);
      continue;
    }
    esp_task_wdt_reset();

    padres.update();
    Serial.println("Padres updated");
    clearSegmentDisplay();
    if (padres.isPlaying)
    {
      // show score
      gameInfo game = padres.currentGame;
      gameScore score = padres.currentGame.score;
      bool padresHome = game.homeTeamId == PADRES_TEAM_ID;
      illuminateNumber(score.homeScore, padresHome ? 0 : 255, padresHome ? 255 : 0, 0, 0);
      illuminateNumber(score.awayScore, padresHome ? 255 : 0, padresHome ? 0 : 255, 0, 2);

      Serial.println("Padres are the home team");
      showSegmentDisplay();

      // SHOW SOME TEXT ON THE ARRAY
      texts[0] = {"Padres(" + String(padres.record.wins) + "-" + String(padres.record.losses) + ") vs " + padres.opponent->teamAbbreviation + " " + padres.opponent->teamName + "(" + String(padres.opponent->record.wins) + "-" + String(padres.opponent->record.losses) + ")", 255, 255, 0};
      texts[1] = {game.inningState + " of " + game.currentInning, 0, 0, 255};

      if (game.inningState == "Top" || game.inningState == "Bottom")
        texts[1].text += " " + (String)game.outs + " OUTS ";
      if (padres.currentGame.hasLastPlay)
      {
        texts[2] = {"LAST PLAY:" + game.lastPlay, 122, 5, 232};
      }
      for (int i = 0; i < 3; i++)
      {
        Serial.println(texts[i].text);
      }
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
      String nextGameStr = "NEXT GAME VS " + (padresHomeNextGame ? padres.nextGame.awayTeam : padres.nextGame.homeTeam) + "(" + (String)padres.nextOpponentRecord.wins + "-" + (String)padres.nextOpponentRecord.losses + ") AT " + padres.nextGame.startTime;
      String probablePitchersStr = "PROBABLE PITCHERS " + padres.nextGame.homeTeamAbbreviation + ":" + padres.nextGame.homeProbablePitcher + " " + padres.nextGame.awayTeamAbbreviation + ":" + padres.nextGame.awayProbablePitcher;
      String gameBackStr = "GAMES BACK NLWEST:" + String(padres.gamesBackFromFirst) + " WC:" + String(padres.gamesBackFromWildcard);
      String divisionRankStr = "DIV RANK:" + String(padres.divisionRank) + " WC RANK:" + String(padres.wildCardRank);
      String streakStr = "Streak:" + padres.streakType + " " + padres.streakNumber;
      String lastTen = "Last 10:" + String(padres.lastTenRecord.wins) + "-" + String(padres.lastTenRecord.losses);
      bool isWinStreak = padres.streakType == "wins";
      texts[0] = {nextGameStr, 255, 255, 0};
      texts[1] = {probablePitchersStr, 122, 5, 232};
      texts[2] = {gameBackStr, 0, 0, 255};
      texts[3] = {divisionRankStr, 7, 191, 247};
      texts[4] = {streakStr, isWinStreak ? 0 : 255, isWinStreak ? 255 : 0, 0};
      texts[5] = {lastTen, 245, 238, 34};

      ledMatrix.showMultiTexts(texts, 6);
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
  // EEPROM_INITIALIZED = false;
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

  setupSegmentDisplay(50);
  ledMatrix.setupMatrix(50);

  WiFi.softAP(access_point_ssid, access_point_password);
  WiFi.softAPsetHostname("padres_scoreboard");
  access_point_ip_address = WiFi.softAPIP();

  xTaskCreatePinnedToCore(serverLoop, "serverLoop", 8092, NULL, 0, serverTask, 1);
  Serial.print("Access point ip address:");
  Serial.println(access_point_ip_address);

  esp_task_wdt_init(30, true);

  xTaskCreatePinnedToCore(
      teamUpdateLoop,   /* Function to implement the task */
      "teamUpdateLoop", /* Name of the task */
      8092,             /* Stack size in words */
      NULL,             /* Task input parameter */
      0,                /* Priority of the task */
      NULL,             /* Task handle. */
      0);               /* Core where the task should run */

  delay(1000);

  // Enable watchdog on Timer 0

  xTaskCreatePinnedToCore(
      ledLoop,   /* Function to implement the task */
      "ledLoop", /* Name of the task */
      4096,      /* Stack size in words */
      NULL,      /* Task input parameter */
      0,         /* Priority of the task */
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