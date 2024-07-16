#include <ESP32_easy_wifi_data.h>
/*
 * This example is a simple controller that sends data to the example_controlled example of whether the built in button is pressed
 */

const int ledPin = 2;
const int buttonPin = 0;

boolean buttonVal = false;
boolean ledVal = false;

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
    EWD::mode = EWD::Mode::connectToNetwork;
    EWD::routerName = "esptest1";
    EWD::routerPassword = "password";
    EWD::routerPort = 25001;
    EWD::communicateWithIP = "192.168.4.1";
    EWD::debugPrint = true;
}

void setup()
{
    pinMode(ledPin, OUTPUT);
    Serial.begin(115200);
    configWifi();
    EWD::setupWifi(WifiDataToParse, WifiDataToSend);
    Serial.println("controller running");
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