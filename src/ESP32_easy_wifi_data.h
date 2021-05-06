#ifndef _ESP32_easy_wifi_data_H_
#define _ESP32_easy_wifi_data_H_
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
namespace EWD {
const char* routerName = "networkName";
const char* routerPass = "networkPass";
const char* APPass = "RCMpassword";
int port = 25210;
int APPort = 25210;
const boolean connectToNetwork = false; //true=try to connect to router  false=go straight to hotspot mode
const boolean wifiRestartNotHotspot = true; //when connection issue, true=retry connection to router  false=fall back to hotspot
const int SIGNAL_LOSS_TIMEOUT = 1000; //disable if no signal after this many milliseconds
const boolean debugPrint = true;
unsigned long lastMessageTimeMillis = 0;

namespace {
    char APName[9];
    WiFiUDP udp;
    const int maxWifiSendBufSize = 100;
    const int maxWifiRecvBufSize = 100;
    boolean wifiConnected = false;
    boolean receivedNewData = false;
    byte recvdData[maxWifiRecvBufSize] = { 0 };
    byte dataToSend[maxWifiSendBufSize] = { 0 };
    int numBytesToSend = 0;
    int wifiArrayCounter = 0;
    IPAddress wifiIPLock = IPAddress(0, 0, 0, 0);

    void (*sendCallback)(void);
    void (*recieveCallback)(void);

    void wifiEvent(WiFiEvent_t event)
    {
        switch (event) {
        case SYSTEM_EVENT_STA_DISCONNECTED:
            if (debugPrint)
                Serial.println("########## router disconnected");
            wifiConnected = false;
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            if (debugPrint) {
                Serial.print("########## wifi connected! IP address: ");
                Serial.print(WiFi.localIP());
                Serial.print(" port: ");
                Serial.println(port);
            }
            udp.begin(port);
            wifiConnected = true;
            break;
        case SYSTEM_EVENT_AP_START:
            if (!wifiConnected) {
                if (debugPrint)
                    Serial.println("########## wifi hotspot started");
                udp.begin(APPort);
            }
            wifiConnected = true;
            break;
        case SYSTEM_EVENT_AP_STACONNECTED:
            if (debugPrint)
                Serial.println("########## client connected to hotspot");
            wifiConnected = true;
            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            if (debugPrint)
                Serial.println("########## client disconnected from hotspot");
            wifiConnected = false;
            break;
        default:
            break;
        }
    }
};

void setupWifi(void (*_recvCP)(void), void (*_sendCP)(void))
{
    sendCallback = _sendCP;
    recieveCallback = _recvCP;
    wifiConnected = false;
    WiFi.disconnect(true);
    sprintf(APName, "RCM%05d", port); // create SSID
    delay(1000);
    WiFi.onEvent(wifiEvent);
    if (connectToNetwork) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(routerName, routerPass);
    }
    delay(3000);
    if (!wifiConnected) {
        if (debugPrint)
            Serial.println("########## connection to router failed/skipped");
        WiFi.disconnect(true);
        if (wifiRestartNotHotspot) {
            ESP.restart();
        }
        WiFi.mode(WIFI_AP);
        if (debugPrint) {
            Serial.println("########## switching to wifi hotspot mode");
            Serial.print("             network name: ");
            Serial.print(APName);
            Serial.print("  password: ");
            Serial.println(APPass);
        }
        delay(1000);
        WiFi.softAP(APName, APPass, 1, 0, 1);
        delay(1000);
        WiFi.softAPConfig(IPAddress(10, 25, 21, 1), IPAddress(10, 25, 21, 1), IPAddress(255, 255, 255, 0));
        delay(1000);
    }
    if (!wifiConnected && debugPrint) {
        Serial.println("########## wifi malfunction, try reboot");
    }
    if (debugPrint)
        Serial.println("########## starting main loop");
}

void runWifiCommunication()
{
    int packetSize = udp.parsePacket();
    if (udp.remoteIP() != IPAddress(0, 0, 0, 0) && wifiIPLock == IPAddress(0, 0, 0, 0)) {
        wifiIPLock = udp.remoteIP();
    }
    if (millis() - lastMessageTimeMillis > SIGNAL_LOSS_TIMEOUT) {
        wifiIPLock = IPAddress(0, 0, 0, 0);
    }
    receivedNewData = false;
    if (packetSize) { //got a message
        udp.read(recvdData, maxWifiRecvBufSize);
        if (udp.remoteIP() == wifiIPLock || wifiIPLock == IPAddress(0, 0, 0, 0)) {
            receivedNewData = true;
            lastMessageTimeMillis = millis();
            for (int i = 0; i < maxWifiRecvBufSize; i++) {
                recvdData[i] = (byte)((int)(256 + recvdData[i]) % 256);
            }
            wifiArrayCounter = 0;
            recieveCallback();
            wifiArrayCounter = 0;
            sendCallback();
            numBytesToSend = wifiArrayCounter;
            udp.beginPacket();
            for (byte i = 0; i < numBytesToSend; i++) {
                udp.write(dataToSend[i]);
            }
            udp.endPacket();
        }
    }
}

boolean recvBl()
{ // return boolean at pos position in recvdData
    byte msg = recvdData[wifiArrayCounter];
    wifiArrayCounter++; // increment the counter for the next value
    return (msg == 1);
}

byte recvBy()
{ // return byte at pos position in recvdData
    byte msg = recvdData[wifiArrayCounter];
    wifiArrayCounter++; // increment the counter for the next value
    return msg;
}

int recvIn()
{ // return int from four bytes starting at pos position in recvdData
    union { // this lets us translate between two variable types (equal size, but one's four bytes in an array, and one's a four byte int)  Reference for unions: https:// www.mcgurrin.info/robots/127/
        byte b[4];
        int v;
    } d; // d is the union, d.b acceses the byte array, d.v acceses the int

    for (int i = 0; i < 4; i++) {
        d.b[i] = recvdData[wifiArrayCounter];
        wifiArrayCounter++;
    }

    return d.v; // return the int form of union d
}

float recvFl()
{ // return float from 4 bytes starting at pos position in recvdData
    union { // this lets us translate between two variable types (equal size, but one's 4 bytes in an array, and one's a 4 byte float) Reference for unions: https:// www.mcgurrin.info/robots/127/
        byte b[4];
        float v;
    } d; // d is the union, d.b acceses the byte array, d.v acceses the float

    for (int i = 0; i < 4; i++) {
        d.b[i] = recvdData[wifiArrayCounter];
        wifiArrayCounter++;
    }

    return d.v;
}

void sendBl(boolean msg)
{ // add a boolean to the tosendData array
    dataToSend[wifiArrayCounter] = msg;
    wifiArrayCounter++;
}

void sendBy(byte msg)
{ // add a byte to the tosendData array
    dataToSend[wifiArrayCounter] = msg;
    wifiArrayCounter++;
}

void sendIn(int msg)
{ // add an int to the tosendData array (four bytes)
    union {
        byte b[4];
        int v;
    } d; // d is the union, d.b acceses the byte array, d.v acceses the int

    d.v = msg; // put the value into the union as an int

    for (int i = 0; i < 4; i++) {
        dataToSend[wifiArrayCounter] = d.b[i];
        wifiArrayCounter++;
    }
}

void sendFl(float msg)
{ // add a float to the tosendData array (four bytes)
    union { // this lets us translate between two variables (equal size, but one's 4 bytes in an array, and one's a 4 byte float Reference for unions: https:// www.mcgurrin.info/robots/127/
        byte b[4];
        float v;
    } d; // d is the union, d.b acceses the byte array, d.v acceses the float

    d.v = msg;

    for (int i = 0; i < 4; i++) {
        dataToSend[wifiArrayCounter] = d.b[i];
        wifiArrayCounter++;
    }
}

};
#endif
