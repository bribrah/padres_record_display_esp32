#include "utils.h"

void customDelay(int ms)
{
    unsigned long start = millis();
    while (millis() - start < ms)
    {
        yield();
    }
};

JsonDocument makeHttpRequest(String url, int &httpCode)
{
    Serial.println("*************************** Making HTTP request to: " + url + " ***************************");
    WiFiClient client;
    HTTPClient http;
    int start = millis();
    int totalStart = millis();

    Serial.println("Making HTTP request to: " + url);

    http.useHTTP10(true);
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json"); // Specify content-type header
    httpCode = http.GET();                              // Send the request
    Serial.print("Finished making request, took: ");
    Serial.println(millis() - start);
    JsonDocument doc;

    if (httpCode == 200)
    {
        Serial.print("HTTP Response code: ");
        Serial.println(httpCode);
        Serial.println("Starting to parse response");
        start = millis();
        ReadBufferingStream bufferedFile(http.getStream(), 64);
        DeserializationError error = deserializeJson(doc, bufferedFile, DeserializationOption::NestingLimit(50));
        Serial.print("Finished parsing response, took: ");
        Serial.println(millis() - start);
        http.end();
        if (error)
        {
            bool overAllocated = doc.overflowed();
            Serial.print("Deserialization error: ");
            Serial.println(error.c_str());
            if (overAllocated)
            {
                Serial.println("Document was overallocated");
            }
        }
    }
    else
    {
        Serial.print("Error code: ");
        Serial.println(httpCode);
    }
    Serial.print("Total time: ");
    Serial.println(millis() - totalStart);
    Serial.println("************************ Finished HTTP request ************************");
    return doc;
}

String mlbTimeToWestCoast(String inputString)
{

    // Extract year, month, day, hour, minute, and second from the input string
    int year = inputString.substring(0, 4).toInt();
    int month = inputString.substring(5, 7).toInt();
    int day = inputString.substring(8, 10).toInt();
    int hour = inputString.substring(11, 13).toInt();
    int minute = inputString.substring(14, 16).toInt();
    int second = inputString.substring(17, 19).toInt();

    struct tm timeinfo;
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = month - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = second;

    time_t timestamp = mktime(&timeinfo);

    const int westCoastOffset = -7 * 3600; // West Coast is 7 hours behind UTC
    timestamp += westCoastOffset;

    struct tm *westCoastTime = localtime(&timestamp);

    char formattedString[20];
    sprintf(formattedString, "%d/%d %02d:%02d %s",
            westCoastTime->tm_mon + 1, westCoastTime->tm_mday,
            westCoastTime->tm_hour % 12 == 0 ? 12 : westCoastTime->tm_hour % 12,
            westCoastTime->tm_min,
            westCoastTime->tm_hour < 12 ? "AM" : "PM");
    return formattedString;
}

void listFiles()
{
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while (file)
    {
        Serial.print("FILE: ");
        Serial.println(file.name());
        file.close();
        file = root.openNextFile();
    }
}

String loadFile(String filename)
{
    File file = SPIFFS.open(filename, "r");
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return "";
    }
    String fileContents = file.readString();
    file.close();
    return fileContents;
}