#include <ESP32_easy_wifi_data.h>
/*
 * This example is a simple controller that sends data to the example_controlled example of whether the built in button is pressed
 */

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
    EWD::routerName = "router";
    EWD::routerPassword = "password";
    EWD::routerPort = 25210;
    EWD::communicateWithIP = "192.168.137.112";
    EWD::debugPrint = true;
}

void setup()
{
    pinMode(2, OUTPUT);
    Serial.begin(115200);
    configWifi();
    EWD::setupWifi(WifiDataToParse, WifiDataToSend);
    Serial.println("controller running");
}

void loop()
{
    buttonVal = digitalRead(0);
    EWD::runWifiCommunication();
    if (EWD::newData()) {
        Serial.println("got new data");
    }
    digitalWrite(2, ledVal);
    delay(10);
}