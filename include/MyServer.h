#pragma once
#include "WiFi.h"
#include "EEPROM.h"
#include "EEPROM_locations.h"
#include "utils.h"
#include "MlbTeam.h"
#include <WebServer.h>
#include <ArduinoJson.h>

void serverLoop(void *parameter);
