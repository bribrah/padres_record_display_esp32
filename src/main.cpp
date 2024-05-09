#include <Arduino.h>
#include <EEPROM.h>
#if defined(ESP8266)
#define HARDWARE "ESP8266"
#include "ESP8266WiFi.h"
#include "ESP8266HTTPClient.h"
#elif defined(ESP32)
#define HARDWARE "ESP32"
#include "WiFi.h"
#include <HTTPClient.h>
#endif

#include <WiFiClient.h>
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

// led matrix defines

int loopNumber = 0;
bool firstTeamUpdateDone = false;
MlbTeam padres(PADRES_TEAM_ID);
LedMatrixDisplay ledMatrix(LED_MATRIX_PIN);
const String access_point_ssid = "PadresScoreboard";
const String access_point_password = "password";
IPAddress access_point_ip_address;
MyServer server(80);

void connect_wifi()
{
  Serial.println("Reading ssid and password from EEPROM");
  int ssid_length = EEPROM.read(SSID_LOCATION);
  int password_length = EEPROM.read(PASSWORD_LOCATION);
  char *ssid_buf = new char[ssid_length + 1];
  char *password_buf = new char[password_length + 1];
  for (int i = 0; i < ssid_length; i++)
  {
    ssid_buf[i] = EEPROM.read(i + 1);
  }
  ssid_buf[ssid_length] = '\0'; // Null terminate the string
  for (int i = 0; i < password_length; i++)
  {
    password_buf[i] = EEPROM.read(i + 1 + PASSWORD_LOCATION);
  }
  password_buf[password_length] = '\0'; // Null terminate the string
  String ssid = String(ssid_buf);
  String password = String(password_buf);
  delete[] ssid_buf;
  delete[] password_buf;

  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
  server.stop();
  Serial.println("connecting wifi");
  WiFi.begin(ssid, password);
  int i = 0;
  ledText text = {"Connecting WIFI...Access Point Ip Address: " + access_point_ip_address.toString(), 0, 0, 255};
  ledMatrix.showSingleText(text);

  while (WiFi.status() != WL_CONNECTED)
  { // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i);
    Serial.print(' ');
    if (i % 2 == 1)
    {
      clearSegmentDisplay();
    }
    else
    {
      changeAllLEDS(255, 196, 37);
    }
    showSegmentDisplay();
  }
  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP()); // Send the IP address of the ESP8266 to the computer
  server.start();
}

void ledLoop(void *pvParameters)
{
  while (true)
  {
    ledMatrix.loopMatrix();
    clearSegmentDisplay();
    if (firstTeamUpdateDone)
    {

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
    }
    delay(MATRIX_UPDATE_INTERVAL);
  }
}

void teamUpdateLoop(void *pvParameters)
{
  while (true)
  {
    Serial.println("WOOT TEST");
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("Wifi not connected, reconnecting...");
      connect_wifi();
    }
    padres.update();
    firstTeamUpdateDone = true;
    delay(TEAM_UPDATE_INTERVAL);
  }
}

///////////////////////// SETUP AND LOOP //////////////////////////
void setup()
{
  Serial.begin(115200);
  setupSegmentDisplay();

  ledMatrix.setupMatrix(LED_MATRIX_BRIGHTNESS);
  // Serial.println("Made it here 2");
  EEPROM.begin(1024); // Initialize EEPROM
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
      0,         /* Priority of the task */
      NULL,      /* Task handle. */
      1);        /* Core where the task should run */
}

void loop()
{
  delay(1000 * 60);
}