#ifndef _ESP32_easy_wifi_data_H_
#define _ESP32_easy_wifi_data_H_
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
namespace EWD {
boolean connectToNetwork = true;
boolean wifiRestartNotHotspot = true;
int signalLossTimeout = 1000;
String routerName = " ";
String routerPass = "-open-network-";
String APPass = "password";
int wifiPort = 25210;
String APName = "ESP32";
int APPort = 25210;
unsigned short connDelay = 2000;
boolean beClientNotServer = false;
IPAddress hotspotAddress = IPAddress(10, 25, 21, 1);
IPAddress serverAddr = IPAddress(10, 25, 21, 1);
boolean blockSimultaneousConnections = true;
boolean debugPrint = true;

#ifndef EWDmaxWifiSendBufSize
#define EWDmaxWifiSendBufSize 41
#endif
#ifndef EWDmaxWifiRecvBufSize
#define EWDmaxWifiRecvBufSize 41
#endif

namespace {
    unsigned long lastMessageTimeMillis = 0;
    unsigned long lastSentMillis = 0;
    WiFiUDP udp;
    boolean wifiConnected = false;
    boolean receivedNewData = false;
    byte recvdData[EWDmaxWifiRecvBufSize] = { 0 };
    byte dataToSend[EWDmaxWifiSendBufSize] = { 0 };
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
            if (beClientNotServer) {
                udp.begin(WiFi.localIP(), wifiPort);
            } else {
                udp.begin(wifiPort);
            }
            if (debugPrint) {
                Serial.print("########## wifi connected! IP address: ");
                Serial.print(WiFi.localIP());
                Serial.print(" wifiPort: ");
                Serial.println(wifiPort);
            }
            wifiConnected = true;
            break;
        case SYSTEM_EVENT_AP_START:
            if (!wifiConnected) {
                udp.begin(APPort);
                if (debugPrint)
                    Serial.println("########## wifi hotspot started");
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

unsigned long millisSinceMessage()
{
    return millis() - lastMessageTimeMillis;
}
bool notTimedOut()
{
    return millis() - lastMessageTimeMillis < signalLossTimeout;
}
bool newData()
{
    return receivedNewData;
}

void setupWifi(void (*_recvCP)(void), void (*_sendCP)(void))
{
    if (debugPrint)
        Serial.println("########## starting setupWifi()");
    sendCallback = _sendCP;
    recieveCallback = _recvCP;
    wifiConnected = false;
    WiFi.disconnect(true);
    delay(1000);
    WiFi.onEvent(wifiEvent);
    if (connectToNetwork) {
        if (debugPrint) {
            Serial.print("connecting to network called: ");
            Serial.print(routerName.c_str());
            Serial.print("  with password: ");
            Serial.println(routerPass.c_str());
        }
        WiFi.mode(WIFI_STA);
        if (strcmp(routerPass.c_str(), "-open-network-") == 0) {
            WiFi.begin(routerName.c_str());
        } else {
            WiFi.begin(routerName.c_str(), routerPass.c_str());
        }
        delay(connDelay);
        if (!wifiConnected) {
            WiFi.disconnect();
            WiFi.reconnect();
            delay(connDelay * 2);
        }
    }
    if (!wifiConnected) {
        if (debugPrint) {
            if (connectToNetwork)
                Serial.println("########## connection to router failed");
        }
        WiFi.disconnect(true);
        if (wifiRestartNotHotspot && connectToNetwork) {
            ESP.restart();
        }
        WiFi.mode(WIFI_AP);
        if (debugPrint) {
            Serial.println("########## switching to wifi hotspot mode");
            Serial.print("             network name: ");
            Serial.print(APName.c_str());
            Serial.print("  password: ");
            Serial.println(APPass.c_str());
            Serial.print("           ip address: ");
            Serial.print(hotspotAddress);
            Serial.print("  port: ");
            Serial.println(APPort);
        }
        delay(1000);
        WiFi.softAP(APName.c_str(), APPass.c_str(), 1, 0, 1);
        delay(1000);
        WiFi.softAPConfig(hotspotAddress, hotspotAddress, IPAddress(255, 255, 255, 0));
        delay(1000);
    }
    if (!wifiConnected && debugPrint) {
        Serial.println("########## wifi malfunction, try reboot");
    }
    if (debugPrint)
        Serial.println("########## wifi setup complete.");
}

void runWifiCommunication()
{
    int packetSize = udp.parsePacket();
    if (blockSimultaneousConnections && udp.remoteIP() != IPAddress(0, 0, 0, 0) && wifiIPLock == IPAddress(0, 0, 0, 0)) {
        wifiIPLock = udp.remoteIP();
    }
    if (blockSimultaneousConnections && millis() - lastMessageTimeMillis > signalLossTimeout) {
        wifiIPLock = IPAddress(0, 0, 0, 0);
    }
    receivedNewData = false;
    if (packetSize) { //got a message
        udp.read(recvdData, EWDmaxWifiRecvBufSize);
        udp.flush();
        if (!blockSimultaneousConnections || udp.remoteIP() == wifiIPLock || wifiIPLock == IPAddress(0, 0, 0, 0)) {
            receivedNewData = true;
            lastMessageTimeMillis = millis();
            lastSentMillis = millis();
            for (int i = 0; i < EWDmaxWifiRecvBufSize; i++) {
                recvdData[i] = (byte)((int)(256 + recvdData[i]) % 256);
            }
            wifiArrayCounter = 0;
            recieveCallback();

            wifiArrayCounter = 0;
            sendCallback();
            numBytesToSend = min(wifiArrayCounter, EWDmaxWifiSendBufSize);
            if (beClientNotServer) {
                udp.beginPacket(serverAddr, wifiPort);
            } else {
                udp.beginPacket();
            }
            for (byte i = 0; i < numBytesToSend; i++) {
                udp.write(dataToSend[i]);
            }
            udp.endPacket();
        }
    } else if (beClientNotServer && millis() - lastSentMillis > signalLossTimeout) {
        lastSentMillis = millis();
        wifiArrayCounter = 0;
        sendCallback();
        numBytesToSend = min(wifiArrayCounter, EWDmaxWifiSendBufSize);
        if (beClientNotServer) {
            udp.beginPacket(serverAddr, wifiPort);
        } else {
            udp.beginPacket();
        }
        for (byte i = 0; i < numBytesToSend; i++) {
            udp.write(dataToSend[i]);
        }
        udp.endPacket();
    }
}

boolean recvBl()
{ // return boolean at pos position in recvdData
    byte msg = recvdData[min(wifiArrayCounter, EWDmaxWifiRecvBufSize - 1)];
    wifiArrayCounter++; // increment the counter for the next value
    return (msg == 1);
}

byte recvBy()
{ // return byte at pos position in recvdData
    byte msg = recvdData[min(wifiArrayCounter, EWDmaxWifiRecvBufSize - 1)];
    wifiArrayCounter++; // increment the counter for the next value
    return msg;
}

int recvIn()
{ // return int from four bytes starting at pos position in recvdData (esp32s use 4 byte ints)
    union { // this lets us translate between two variable types (equal size, but one's four bytes in an array, and one's a four byte int)  Reference for unions: https://www.mcgurrin.info/robots/127/
        byte b[4];
        int v;
    } d; // d is the union, d.b acceses the byte array, d.v acceses the int

    for (int i = 0; i < 4; i++) {
        d.b[i] = recvdData[min(wifiArrayCounter, EWDmaxWifiRecvBufSize - 1)];
        wifiArrayCounter++;
    }

    return d.v; // return the int form of union d
}

float recvFl()
{ // return float from 4 bytes starting at pos position in recvdData
    union { // this lets us translate between two variable types (equal size, but one's 4 bytes in an array, and one's a 4 byte float) Reference for unions: https://www.mcgurrin.info/robots/127/
        byte b[4];
        float v;
    } d; // d is the union, d.b acceses the byte array, d.v acceses the float

    for (int i = 0; i < 4; i++) {
        d.b[i] = recvdData[min(wifiArrayCounter, EWDmaxWifiRecvBufSize - 1)];
        wifiArrayCounter++;
    }

    return d.v;
}

void sendBl(boolean msg)
{ // add a boolean to the tosendData array
    dataToSend[min(wifiArrayCounter, EWDmaxWifiSendBufSize - 1)] = msg;
    wifiArrayCounter++;
}

void sendBy(byte msg)
{ // add a byte to the tosendData array
    dataToSend[min(wifiArrayCounter, EWDmaxWifiSendBufSize - 1)] = msg;
    wifiArrayCounter++;
}

void sendIn(int msg)
{ // add an int to the tosendData array (four bytes, esp32s use 4 byte ints)
    union {
        byte b[4];
        int v;
    } d; // d is the union, d.b acceses the byte array, d.v acceses the int (equal size, but one's 4 bytes in an array, and one's a 4 byte int Reference for unions: https://www.mcgurrin.info/robots/127/

    d.v = msg; // put the value into the union as an int

    for (int i = 0; i < 4; i++) {
        dataToSend[min(wifiArrayCounter, EWDmaxWifiSendBufSize - 1)] = d.b[i];
        wifiArrayCounter++;
    }
}

void sendFl(float msg)
{ // add a float to the tosendData array (four bytes)
    union { // this lets us translate between two variables (equal size, but one's 4 bytes in an array, and one's a 4 byte float Reference for unions: https://www.mcgurrin.info/robots/127/
        byte b[4];
        float v;
    } d; // d is the union, d.b acceses the byte array, d.v acceses the float

    d.v = msg;

    for (int i = 0; i < 4; i++) {
        dataToSend[min(wifiArrayCounter, EWDmaxWifiSendBufSize - 1)] = d.b[i];
        wifiArrayCounter++;
    }
}

};
#endif
