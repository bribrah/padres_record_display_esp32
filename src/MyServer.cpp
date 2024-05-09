#include "MyServer.h"

MyServer::MyServer(int port) : server(port)
{
}

void MyServer::start()
{
    xTaskCreatePinnedToCore(
        serverLoopWrapper, /* Function to implement the task */
        "serverLoop",      /* Name of the task */
        10000,             /* Stack size in words */
        this,              /* Task input parameter */
        0,                 /* Priority of the task */
        NULL,              /* Task handle. */
        1);                /* Core where the task should run */
}
void MyServer::serverLoopWrapper(void *pvParameters)
{
    MyServer *server = static_cast<MyServer *>(pvParameters);
    server->serverLoop();
}

void MyServer::stop()
{
    killServer = true;
}

void MyServer::serverLoop()
{
    Serial.println("Starting server");
    server.begin();
    killServer = false;
    while (!killServer)
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
    server.stop();
    killServer = false;
}