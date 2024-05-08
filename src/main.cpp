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
#include "Adafruit_NeoPixel.h"
#include "ledMatrixDisplay.h"
#include "MlbTeam.h"
#include "EEPROM_locations.h"

// assume 7 segment display and each segment has 4 LEDS

// 7 segmemnt display defines
#define NUM_DIGITS 2
#define NUM_LEDS 4 * 7 * 4
#define LED_PIN 13
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define LED_NUMBER_BRIGHTNESS 128
// task defines
#define TEAM_UPDATE_INTERVAL 7500
#define MATRIX_UPDATE_INTERVAL 15

// led matrix defines
#define LED_MATRIX_PIN 12
#define LED_MATRIX_BRIGHTNESS 256 / 4

Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

int loopNumber = 0;
MlbTeam padres(PADRES_TEAM_ID);
LedMatrixDisplay ledMatrix(LED_MATRIX_PIN);

WiFiServer server(80);

int numberSegmentLookup[10][7] = {
    {1, 2, 3, 4, 5, 6, -1},     // 0
    {1, 6, -1, -1, -1, -1, -1}, // 1
    {0, 1, 2, 4, 5, -1, -1},    // 2
    {0, 1, 2, 5, 6, -1, -1},    // 3
    {0, 1, 3, 6, -1, -1, -1},   // 4
    {0, 2, 3, 5, 6, -1, -1},    // 5
    {0, 2, 3, 4, 5, 6, -1},     // 6
    {1, 2, 6, -1, -1, -1, -1},  // 7
    {0, 1, 2, 3, 4, 5, 6},      // 8
    {0, 1, 2, 3, 5, 6, -1},     // 9
};

void changeAllLEDS(int r, int g, int b)
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    pixels.setPixelColor(i, pixels.Color(r, g, b));
  }
}

void illuminateSegment(int digit, int segment, int r, int g, int b)
{

  int start = (digit * 4 * 7) + segment * 4;
  for (int i = start; i < start + 4; i++)
  {
    pixels.setPixelColor(i, pixels.Color(r, g, b));
  }
}

void illuminateNumber(int number, int r, int g, int b, int digitOffset)
{
  if (number >= 100)
  {
    return;
  }
  else if (number >= 10)
  {
    int secondDigit = number % 10;
    int firstDigit = number / 10;
    for (int i = 0; i < 7; i++)
    {
      if (numberSegmentLookup[firstDigit][i] != -1)
      {
        illuminateSegment(0 + digitOffset, numberSegmentLookup[firstDigit][i], r, g, b);
      }
      if (numberSegmentLookup[secondDigit][i] != -1)
      {
        illuminateSegment(1 + digitOffset, numberSegmentLookup[secondDigit][i], r, g, b);
      }
    }
  }
  else
  {
    for (int i = 0; i < 7; i++)
    {
      if (numberSegmentLookup[number][i] == -1)
      {
        break;
      }
      illuminateSegment(1 + digitOffset, numberSegmentLookup[number][i], r, g, b);
    }
  }
}

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

  Serial.println("connecting wifi");
  WiFi.begin(ssid, password);
  int i = 0;
  ledText text = {"Connecting WIFI...", 0, 0, 255};
  ledMatrix.showSingleText(text);

  while (WiFi.status() != WL_CONNECTED)
  { // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i);
    Serial.print(' ');
    if (i % 2 == 1)
    {
      pixels.clear();
    }
    else
    {
      changeAllLEDS(255, 196, 37);
    }
    pixels.show();
  }
  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP()); // Send the IP address of the ESP8266 to the computer
  server.begin();
}

void ledLoop(void *pvParameters)
{
  while (true)
  {
    ledMatrix.loopMatrix();
    if (WiFi.status() == WL_CONNECTED)
    {
      pixels.clear();
      if (padres.isPlaying)
      {
        // show score
        gameInfo game = padres.currentGame;
        gameScore score = padres.score;
        bool padresHome = game.homeTeamId == PADRES_TEAM_ID;
        illuminateNumber(score.homeScore, padresHome ? 0 : 255, padresHome ? 255 : 0, 0, 0);
        illuminateNumber(score.awayScore, padresHome ? 255 : 0, padresHome ? 0 : 255, 0, 2);
        Serial.println("Padres are the home team");
        pixels.show();

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
        pixels.show();

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

    delay(TEAM_UPDATE_INTERVAL);
  }
}

void serverLoop(void *pvParameters)
{
  while (true)
  {
    WiFiClient client = server.available(); // Checking for incoming clients

    if (client)
    {
      Serial.println("new client");
      String currentLine = ""; // Storing the incoming data in the string
      while (client.connected())
      {
        if (client.available()) // if there is some client data available
        {
          char c = client.read(); // read a byte
          Serial.print(c);
          if (c == '\n') // check for newline character,
          {
            if (currentLine.length() == 0) // if line is blank it means its the end of the client HTTP request
            {
              int length = EEPROM.read(0);
              Serial.print("Length: ");
              Serial.println(length);
              char *buf = new char[length + 1];
              for (int i = 0; i < length; i++)
              {
                buf[i] = EEPROM.read(i + 1);
              }
              buf[length] = '\0'; // Null terminate the string
              Serial.print("EEPROM Data: ");
              Serial.println(buf);
              client.print("<title>ESP32 Webserver</title>");
              client.print("<body><h1>Hello World </h1>");
              client.print("<h2>EEPROM Data: ");
              client.print(buf);
              client.print("</h2></body>");
              delete[] buf;

              break; // Going out of the while loop
            }
            else
            {
              currentLine = ""; // if you got a newline, then clear currentLine
            }
          }
          else if (c != '\r')
          {
            currentLine += c; // if you got anything else but a carriage return character,
          }
        }
      }
    }
    delay(100);
  }
}

///////////////////////// SETUP AND LOOP //////////////////////////
void setup()
{
  Serial.begin(115200);
  pixels.begin();
  pixels.setBrightness(LED_NUMBER_BRIGHTNESS);
  ledMatrix.setupMatrix(LED_MATRIX_BRIGHTNESS);
  // Serial.println("Made it here 2");
  EEPROM.begin(1024); // Initialize EEPROM

  Serial.println("Writing ssid and password to EEPROM");

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

  xTaskCreatePinnedToCore(
      serverLoop,   /* Function to implement the task */
      "serverLoop", /* Name of the task */
      10000,        /* Stack size in words */
      NULL,         /* Task input parameter */
      0,            /* Priority of the task */
      NULL,         /* Task handle. */
      1);           /* Core where the task should run */
}

void loop()
{
  delay(1000 * 60);
}