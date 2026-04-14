#include "MyServer.h"

WebServer server(80);
extern MlbTeam padres;

void handleIndex()
{
    String index = loadFile("/index.html");
    if (index.isEmpty())
    {
        server.send(500, "text/plain", "Failed to load index.html");
        return;
    }

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
    float totalGames = (float)padres.record.wins + (float)padres.record.losses;
    obj["winningPercentage"] = totalGames > 0 ? (float)padres.record.wins / totalGames : 0.0f;
    obj["gamesBackDiv"] = padres.gamesBackFromFirst;
    obj["gamesBackDivision"] = padres.gamesBackFromFirst;
    obj["gamesBackWC"] = padres.gamesBackFromWildcard;
    obj["divisionRank"] = padres.divisionRank;
    obj["leagueRank"] = padres.leagueRank;
    obj["wildCardRank"] = padres.wildCardRank;
    obj["streakType"] = padres.streakType;
    obj["streakNumber"] = padres.streakNumber;
    obj["streak"] = padres.streakType + " " + String(padres.streakNumber);
    obj["runsAllowed"] = padres.runsAllowed;
    obj["runsScored"] = padres.runsScored;
    obj["runDifferential"] = padres.runsScored - padres.runsAllowed;

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
    EEPROM.commit();
    server.send(200, "text/html", "SSID set to: " + ssid);
    delay(100);
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
