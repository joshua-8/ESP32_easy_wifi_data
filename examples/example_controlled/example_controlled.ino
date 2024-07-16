#include <Arduino.h>
#include <ESP32_easy_wifi_data.h>
/*
 * This example is a simple wifi controlled "robot" that turns on the built in led when told to.
 */

const int ledPin = 2;
const int buttonPin = 0;

boolean ledVal = false;
boolean buttonVal = false;

// data receive callback
void WifiDataToParse()
{
    ledVal = EWD::recvBl();
}
// data send callback
void WifiDataToSend()
{
    EWD::sendBl(buttonVal);
}

void configWifi()
{
    EWD::mode = EWD::Mode::createAP;
    EWD::APPort = 25001;
    EWD::APName = "esptest1";
    EWD::APPassword = "password";
}

void setup()
{
    pinMode(ledPin, OUTPUT);
    Serial.begin(115200);
    configWifi();
    EWD::setupWifi(WifiDataToParse, WifiDataToSend);
    Serial.println("example_controlled running");
}
void loop()
{
    buttonVal = digitalRead(buttonPin);
    EWD::runWifiCommunication();
    if (EWD::newData()) {
        Serial.println("got new data");
    }
    digitalWrite(ledPin, ledVal);
    delay(10);
}