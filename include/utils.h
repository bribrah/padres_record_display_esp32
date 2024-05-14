#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "utils.h"
#include "datatypes.h"
#include <StreamUtils.h>
#include <SPIFFS.h>
#include <FS.h>

void customDelay(int ms);
JsonDocument makeHttpRequest(String url, int &httpCode);
JsonDocument makeHttpRequest(String url, JsonDocument filter, int &httpCode);
String mlbTimeToWestCoast(String inputString);
void listFiles();
String loadFile(String filename);