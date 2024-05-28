#include "MyServer.h"

WebServer server(80);
extern MlbTeam padres;

void handleIndex()
{
    String index = loadFile("/index.html");
    server.send(200, "text/html", index);
}

void getRecord()
{
    Serial.println("Getting Record");
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    obj["team"] = padres.fullName;
    obj["wins"] = padres.record.wins;
    obj["losses"] = padres.record.losses;
    obj["winningPercentage"] = (float)padres.record.wins / ((float)padres.record.wins + (float)padres.record.losses);
    obj["gamesBackDiv"] = padres.gamesBackFromFirst;
    obj["gamesBackWC"] = padres.gamesBackFromWildcard;
    obj["divisionRank"] = padres.divisionRank;
    obj["leagueRank"] = padres.leagueRank;
    obj["wildCardRank"] = padres.wildCardRank;
    obj["streakType"] = padres.streakType;
    obj["streakNumber"] = padres.streakNumber;
    obj["runsAllowed"] = padres.runsAllowed;
    obj["runsScored"] = padres.runsScored;

    String response;
    serializeJson(obj, response);
    server.send(200, "application/json", response);
}

void getWifiStatus()
{
    String ssid = EEPROM.readString(SSID_LOCATION);
    String connected = WiFi.isConnected() ? "Connected" : "Not Connected";
    String IP = WiFi.localIP().toString();
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    obj["ssid"] = ssid;
    obj["status"] = connected;
    obj["ip"] = IP;
    String response;
    serializeJson(obj, response);
    server.send(200, "application/json", response);
}

void setSSID()
{
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    EEPROM.writeString(SSID_LOCATION, ssid);
    EEPROM.writeString(PASSWORD_LOCATION, password);
    server.send(200, "text/html", "SSID set to: " + ssid);
    WiFi.disconnect();
}

void serverLoop(void *parameter)
{

    server.on("/", HTTP_GET, handleIndex);
    server.on("/record", HTTP_GET, getRecord);
    server.on("/wifi", HTTP_GET, getWifiStatus);
    server.on("/wifi", HTTP_POST, setSSID);

    server.begin();
    Serial.println("HTTP server started");
    while (true)
    {
        server.handleClient();
        delay(1);
    }
};