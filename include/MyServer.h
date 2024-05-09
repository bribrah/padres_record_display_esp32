#pragma once
#include "WiFi.h"
#include "EEPROM.h"
#include "EEPROM_locations.h"

class MyServer
{

public:
    MyServer(int port);
    void start();
    void stop();

private:
    WiFiServer server;
    static void serverLoopWrapper(void *pvParameters);
    void serverLoop();

    bool killServer = false;
};
