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
    obj["team"] = padres.teamName;
    obj["wins"] = padres.record.wins;
    obj["losses"] = padres.record.losses;
    String response;
    serializeJson(obj, response);
    server.send(200, "application/json", response);
}
void serverLoop(void *parameter)
{

    server.on("/", HTTP_GET, handleIndex);
    server.on("/record", HTTP_GET, getRecord);

    server.begin();
    Serial.println("HTTP server started");
    while (true)
    {
        server.handleClient();
    }
};