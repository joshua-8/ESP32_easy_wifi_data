#include <Arduino.h>
#include <ESP32_easy_wifi_data.h>
void setup()
{
    EWD::setupWifi(EWDdataRecieveCallback, EWDdataSendCallback);
    // put your setup code here, to run once:
}

void loop()
{
    EWD::runWifiCommunication();
    // put your main code here, to run repeatedly:
}
void EWDdataRecieveCallback()
{
}
void EWDdataSendCallback()
{
    Serial.println("SEND CALLBACK !!!!!!!!!!!!!!!");
}
