#include <Arduino.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <secrets.h>

WiFiUDP ntpUDP;

// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
NTPClient timeClient(ntpUDP);

// You can specify the time server pool and the offset, (in seconds)
// additionally you can specify the update interval (in milliseconds).
// NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

void setup()
{
    Serial.begin(115200);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        // Serial.print(".");
    }

    timeClient.begin();
    timeClient.update();
    Serial.println(timeClient.getFormattedTime());
}

unsigned long millisLast = 0;
void loop()
{
    timeClient.update();
    // send the time once a minute
    if(timeClient.isTimeSet() && millis() - millisLast > 60000){
        millisLast = millis();
        Serial.println(timeClient.getFormattedTime());
    }
    delay(1000);
}