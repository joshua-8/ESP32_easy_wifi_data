/*
    example for ESP32_easy_wifi_data library
    https://github.com/joshua-8/ESP32_easy_wifi_data
    This example shows every option and function of the library.
*/
#include <Arduino.h>

#define EWDmaxWifiSendBufSize 10 //how many bytes will your outgoing message be? (boolean=1,byte=1,int=4,float=4) (remove this line to use the default of 40)
#define EWDmaxWifiRecvBufSize 10 //how many bytes will your incoming message be? (boolean=1,byte=1,int=4,float=4) (remove this line to use the default of 40)
#include <ESP32_easy_wifi_data.h> //https://github.com/joshua-8/ESP32_easy_wifi_data

void setup()
{
    Serial.begin(9600);

    EWD::connectToNetwork = true; //true=try to connect to router  false=go straight to hotspot mode (default: true)
    EWD::wifiRestartNotHotspot = true; //when connection issue, true=retry connection to router  false=fall back to hotspot (default: true)
    EWD::signalLossTimeout = 1000; //disable if no signal after this many milliseconds (default: 1000)
    EWD::routerName = " "; //name of the wifi network you want to connect to
    EWD::routerPass = "-open-network-"; //password for your wifi network (enter "-open-network-" if the network has no password) (default: -open-network-)
    EWD::APPass = "password"; //password for network created by esp32 if it goes into hotspot mode (default: password)
    EWD::wifiPort = 25210; //what port the esp32 communicates on if connected to a wifi network (default: 25210)
    EWD::APName = "ESP32"; //name of hotspot network (default: ESP32)
    EWD::APPort = 25210; //what port the esp32 communicates on if it makes its own network (default 25210)
    EWD::connDelay = 2000; //milliseconds to wait for the router to connect, try increasing if having trouble connecting (default 2000)
    EWD::beClientNotServer = false; //connect to server at ip: serverAddr and port: wifiPort instead of creating a server (default: false)
    EWD::hotspotAddress = IPAddress(10, 25, 21, 1); //if the esp32 creates a hotspot, what address should it give itself on its network (default: 10.25.21.1)
    EWD::serverAddr = IPAddress(10, 25, 21, 1); //ip address to contact server on if beClientNotServer is true (default: 10.25.21.1)
    EWD::blockSimultaneousConnections = true; //block more than one client from connecting at a time (default: true)
    EWD::debugPrint = true; //print ip address and other information to the serial monitor (default: true)

    EWD::setupWifi(dataRecieveCallback, dataSendCallback); //this function connects to wifi, it may take up to 10 seconds
}

void loop()
{
    EWD::runWifiCommunication(); //call in loop, as often as possible in order to stay connected
    EWD::millisSinceMessage(); //returns how many milliseconds it has been since a message was last recieved
    EWD::notTimedOut(); //returns millisSinceMessage<signalLossTimeout, use to disable your robot if it loses connection
    EWD::newData(); //returns true if new data was recieved this loop
}
void dataRecieveCallback()
{
    Serial.print(EWD::recvBl()); //recieve a boolean
    Serial.print(",");
    Serial.print(EWD::recvBy()); //recieve a byte
    Serial.print(",");
    Serial.print(EWD::recvIn()); //recieve an int
    Serial.print(",");
    Serial.print(EWD::recvFl()); //recieve a float
    Serial.println();
}
void dataSendCallback()
{
    EWD::sendBl(true); //send a boolean
    EWD::sendBy(255); //send a byte
    EWD::sendIn(0); //send an int
    EWD::sendFl(1.0); //send a float
}
