#include <Arduino.h>

#define EWDmaxWifiSendBufSize 8 //how many bytes will your outgoing message be? (boolean=1,byte=1,int=4,float=4) (remove this line to use the default of 40)
#define EWDmaxWifiRecvBufSize 8 //how many bytes will your incoming message be? (boolean=1,byte=1,int=4,float=4) (remove this line to use the default of 40)
#include <ESP32_easy_wifi_data.h>

void setup()
{
    Serial.begin(9600);
    EWD::connectToNetwork = true; //true=try to connect to router  false=go straight to hotspot mode (default: false)
    EWD::wifiRestartNotHotspot = false; //when connection issue, true=retry connection to router  false=fall back to hotspot (default: false)
    EWD::signalLossTimeout = 1000; //disable if no signal after this many milliseconds (default: 1000)
    EWD::routerName = "chicken"; //name of the wifi network you want to connect to
    EWD::routerPass = "bawkbawk"; //password for your wifi network (enter "-open-network-" if the network has no password) (default: -open-network-)
    EWD::APPass = "mypassword"; //password for network created by esp32 if it goes into hotspot mode (default: password)
    EWD::wifiPort = 25218; //what port the esp32 communicates on if connected to a wifi network (default: 25210)
    EWD::APPort = 25210; //what port the esp32 communicates on if it makes its own network (default 25210)
    EWD::beClientNotServer = true; //connect to server at ip: serverAddr and port: wifiPort instead of creating a server (default: false)
    EWD::hotspotAddress = IPAddress(10, 25, 21, 0); //if the esp32 creates a hotspot, what address should it give itself on its network (default: 10.25.21.1)
    EWD::serverAddr = IPAddress(10, 0, 0, 24); //ip address to contact server on if beClientNotServer is true (default: 10.25.21.1)
    EWD::blockSimultaneousConnections = false; //block more than one client from connecting at a time (default: true)
    EWD::debugPrint = true; //print ip address and other information to the serial monitor (default: true)

    EWD::setupWifi(dataRecieveCallback, dataSendCallback); //this function connects to wifi, it may take up to about 10 seconds
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
    EWD::recvBl(); //recieve a boolean
    EWD::recvBy(); //recieve a byte
    EWD::recvIn(); //recieve an int
    EWD::recvFl(); //recieve a float
}
void dataSendCallback()
{
    EWD::sendBl(true); //send a boolean
    EWD::sendBy(180); //send a byte
    EWD::sendIn(654321); //send an int
    EWD::sendFl(9.8); //send a float
}
