/*
    An example for ESP32_easy_wifi_data library
    https://github.com/joshua-8/ESP32_easy_wifi_data
    This example shows every option and function of the library but is not meant to do anything useful if run as is.
    by joshua-8, 2021-2022, MIT License
*/
#include <Arduino.h>

#define EWDmaxWifiSendBufSize 41 // how many bytes will your outgoing message be? (boolean=1,byte=1,int=4,float=4) (remove this line to use the default of 41)
#define EWDmaxWifiRecvBufSize 41 // how many bytes will your incoming message be? (boolean=1,byte=1,int=4,float=4) (remove this line to use the default of 41)
#include <ESP32_easy_wifi_data.h> //https://github.com/joshua-8/ESP32_easy_wifi_data

void setup()
{
    Serial.begin(9600);

    EWD::mode = EWD::Mode::connectToNetwork; // "connectToNetwork" or "createAP"(hotspot). default: connect to network
    EWD::signalLossTimeout = 1000; // connection times out if no signal after this many milliseconds (default: 1000 (= 1 sec))
    EWD::routerName = " "; // name of the wifi network you want to connect to
    EWD::routerPassword = ""; // password for your wifi network (enter "" if the network has no password) (default: "")
    EWD::APName = "esp32"; // name of hotspot network (default: esp32)
    EWD::APPassword = "password"; // password for network created by esp32 if it is in hotspot mode (default: password)
    EWD::routerPort = 25210; // what port the esp32 communicates on if connected to a wifi network (default: 25210)
    EWD::APPort = 25210; // what port the esp32 communicates on if it makes its own network (default 25210)
    EWD::blockSimultaneousConnections = true; // block more than one client from connecting at a time (default: true)
    EWD::debugPrint = false; // print ip address and other information to the serial monitor (default: false)

    EWD::setupWifi(dataReceiveCallback, dataSendCallback); // this function connects to wifi, it may take up to 10 seconds, give it the names of dataReceiveCallback, dataSendCallback (functions with data sending and data receiving commands)
}

void loop()
{
    EWD::runWifiCommunication(); // call in loop, as often as possible in order to stay connected
    EWD::millisSinceMessage(); // returns how many milliseconds it has been since a message was last received
    EWD::notTimedOut(); // returns millisSinceMessage<signalLossTimeout
    EWD::timedOut(); // returns millisSinceMessage>=signalLossTimeout, >>>use to disable your robot if it loses connection<<<
    EWD::newData(); // returns true if new data was received this loop
    EWD::getWifiConnected(); // returns true if the wifi connection has been started to the router or a hotspot has been made (unrelated to whether data has been received, for that use notTimedOut)
}
void dataReceiveCallback()
{
    boolean a = EWD::recvBl(); // receive a boolean
    byte b = EWD::recvBy(); // receive a byte
    int c = EWD::recvIn(); // receive an int  (int32_t)
    float d = EWD::recvFl(); // receive a float
}
void dataSendCallback()
{
    EWD::sendBl(true); // send a boolean
    EWD::sendBy(255); // send a byte
    EWD::sendIn(0); // send an int  (int32_t)
    EWD::sendFl(1.0); // send a float
}
