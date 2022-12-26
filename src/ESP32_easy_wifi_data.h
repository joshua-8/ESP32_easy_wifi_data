#ifndef _ESP32_easy_wifi_data_H_
#define _ESP32_easy_wifi_data_H_

#include <Arduino.h>

#include <WiFi.h>
#include <WiFiUdp.h>

namespace EWD {

WiFiUDP udp;

enum Mode { createAP,
    connectToNetwork };

Mode mode = connectToNetwork;
int signalLossTimeout = 1000;
const char* routerName = " ";
const char* routerPassword = "";
const char* APName = "esp32";
const char* APPassword = "password";
int routerPort = 25210;
int APPort = 25210;

boolean blockSimultaneousConnections = true;
boolean debugPrint = false;

#ifndef EWDmaxWifiSendBufSize
#define EWDmaxWifiSendBufSize 41
#endif
#ifndef EWDmaxWifiRecvBufSize
#define EWDmaxWifiRecvBufSize 41
#endif

// variables below this line should not be modified
unsigned long lastMessageTimeMillis = 0;
unsigned long lastSentMillis = 0;
boolean wifiConnected = false;
boolean receivedNewData = false;
byte recvdData[EWDmaxWifiRecvBufSize] = { 0 };
byte dataToSend[EWDmaxWifiSendBufSize] = { 0 };
int wifiArrayCounter = 0;
IPAddress wifiIPLock = IPAddress(0, 0, 0, 0);

void (*sendCallback)(void);
void (*receiveCallback)(void);

void WiFiEvent(WiFiEvent_t event)
{

    if (debugPrint)
        Serial.printf("[EWD]  event %d:  ", event);

    switch (event) {
    case ARDUINO_EVENT_WIFI_READY:
        if (debugPrint)
            Serial.println("WiFi interface ready");
        break;
    case ARDUINO_EVENT_WIFI_STA_START:
        if (debugPrint)
            Serial.println("WiFi client started");
        break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
        if (debugPrint)
            Serial.println("WiFi clients stopped");
        break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        if (debugPrint)
            Serial.println("Connected to access point");
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        if (debugPrint)
            Serial.println("Disconnected from WiFi access point");

        if (wifiConnected) {
            if (debugPrint)
                Serial.println("[EWD]  Attempting to reconnect");
            WiFi.reconnect();
        }
        wifiConnected = false;
        break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        if (debugPrint)
            Serial.printf("[EWD]  event %d:  ", event);
        Serial.print("Obtained IP address: ");
        Serial.println(WiFi.localIP());
        wifiConnected = true;
        udp.begin(routerPort);
        break;
    case ARDUINO_EVENT_WIFI_AP_START:
        if (debugPrint)
            Serial.println("WiFi access point started");

        udp.begin(APPort);
        if (debugPrint)
            Serial.println("[EWD]  UDP begin - AP mode");
        wifiConnected = true;

        break;
    case ARDUINO_EVENT_WIFI_AP_STOP: // not sure what causes this, if you see it, it's probably bad, try restarting the esp32
        if (debugPrint)
            Serial.println("WiFi access point stopped");
        break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
        if (debugPrint)
            Serial.println("Client connected");
        break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
        if (debugPrint)
            Serial.println("Client disconnected");
        break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
        if (debugPrint)
            Serial.println("Assigned IP address to client");
        break;
    default:
        break;
    }
}

void sendMessage()
{
    lastSentMillis = millis();
    wifiArrayCounter = 0;
    sendCallback();

    udp.beginPacket();
    for (byte i = 0; i < min(wifiArrayCounter, EWDmaxWifiSendBufSize); i++) {
        udp.write(dataToSend[i]);
    }
    udp.endPacket();
}

void setupWifi(void (*_recvCB)(void), void (*_sendCB)(void))
{
    WiFi.disconnect(true, true);
    delay(100);

    WiFi.onEvent(WiFiEvent);

    if (debugPrint)
        Serial.println("[EWD] starting setupWifi()");
    sendCallback = _sendCB;
    receiveCallback = _recvCB;

    if (mode == Mode::createAP) {
        Serial.printf("[EWD] creating AP called %s with password %s \n", APName, APPassword);

        WiFi.softAP(APName, APPassword);
    }
    if (mode == Mode::connectToNetwork) {
        if (strlen(routerPassword) < 1) {
            Serial.printf("[EWD]  Attempting to connect to open network %s \n", routerName);
            WiFi.begin(routerName);
        } else {
            Serial.printf("[EWD]  Attempting to connect to %s with password %s \n", routerName, routerPassword);
            WiFi.begin(routerName, routerPassword);
        }
    }
}

void runWifiCommunication()
{
    receivedNewData = false;

    if (!wifiConnected) {
        return;
    }
    int packetSize = udp.parsePacket();

    if (blockSimultaneousConnections && udp.remoteIP() != IPAddress(0, 0, 0, 0) && wifiIPLock == IPAddress(0, 0, 0, 0)) {
        wifiIPLock = udp.remoteIP(); // lock
    }
    if (blockSimultaneousConnections && millis() - lastMessageTimeMillis > signalLossTimeout) {
        wifiIPLock = IPAddress(0, 0, 0, 0); // unlock
    }

    if (packetSize) { // got a message
        udp.read(recvdData, EWDmaxWifiRecvBufSize);
        udp.flush();

        if (blockSimultaneousConnections && udp.remoteIP() != wifiIPLock && wifiIPLock != IPAddress(0, 0, 0, 0)) {
            return; // block other IP addresses
        }

        receivedNewData = true;
        lastMessageTimeMillis = millis();
        for (int i = 0; i < EWDmaxWifiRecvBufSize; i++) {
            recvdData[i] = (byte)((int)(256 + recvdData[i]) % 256);
        }
        wifiArrayCounter = 0;
        receiveCallback();

        sendMessage();
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

//  great reference on unions, which make this code work: https://www.mcgurrin.info/robots/127/
int recvIn()
{ // return int from four bytes starting at pos position in recvdData (esp32s use 4 byte ints)
    union { // this lets us translate between two variable types (equal size, but one's four bytes in an array, and one's a four byte int)  Reference for unions: https://www.mcgurrin.info/robots/127/
        byte b[4];
        int v;
    } d; // d is the union, d.b accesses the byte array, d.v accesses the int

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
    } d; // d is the union, d.b accesses the byte array, d.v accesses the float

    for (int i = 0; i < 4; i++) {
        d.b[i] = recvdData[min(wifiArrayCounter, EWDmaxWifiRecvBufSize - 1)];
        wifiArrayCounter++;
    }

    return d.v;
}

void sendBl(boolean msg)
{ // add a boolean to the dataToSend array
    dataToSend[min(wifiArrayCounter, EWDmaxWifiSendBufSize - 1)] = msg;
    wifiArrayCounter++;
}

void sendBy(byte msg)
{ // add a byte to the dataToSend array
    dataToSend[min(wifiArrayCounter, EWDmaxWifiSendBufSize - 1)] = msg;
    wifiArrayCounter++;
}

//  great reference on unions, which make this code work: https://www.mcgurrin.info/robots/127/
void sendIn(int msg)
{ // add an int to the dataToSend array (four bytes, esp32s use 4 byte ints)
    union {
        byte b[4];
        int v;
    } d; // d is the union, d.b accesses the byte array, d.v accesses the int (equal size, but one's 4 bytes in an array, and one's a 4 byte int Reference for unions: https://www.mcgurrin.info/robots/127/

    d.v = msg; // put the value into the union as an int

    for (int i = 0; i < 4; i++) {
        dataToSend[min(wifiArrayCounter, EWDmaxWifiSendBufSize - 1)] = d.b[i];
        wifiArrayCounter++;
    }
}

void sendFl(float msg)
{ // add a float to the dataToSend array (four bytes)
    union { // this lets us translate between two variables (equal size, but one's 4 bytes in an array, and one's a 4 byte float Reference for unions: https://www.mcgurrin.info/robots/127/
        byte b[4];
        float v;
    } d; // d is the union, d.b accesses the byte array, d.v accesses the float

    d.v = msg;

    for (int i = 0; i < 4; i++) {
        dataToSend[min(wifiArrayCounter, EWDmaxWifiSendBufSize - 1)] = d.b[i];
        wifiArrayCounter++;
    }
}

unsigned long millisSinceMessage()
{
    return millis() - lastMessageTimeMillis;
}
bool notTimedOut()
{
    return millis() - lastMessageTimeMillis < signalLossTimeout;
}
bool timedOut()
{
    return millis() - lastMessageTimeMillis >= signalLossTimeout;
}
bool newData()
{
    return receivedNewData;
}
bool getWifiConnected()
{
    return wifiConnected;
}
};
#endif // include guard