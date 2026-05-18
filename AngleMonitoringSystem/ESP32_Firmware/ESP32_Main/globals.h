#pragma once

#include <WiFi.h>
#include <WiFiUdp.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <Wire.h>

#define duetSerial Serial2
#define duetBaud   115200
#define LED_PIN 2

using namespace imu;

extern sensors_event_t event;

// IMU OBJECTS
extern Adafruit_BNO055 bno;
extern WiFiUDP udp;

// WIFI SETTINGS
extern const char* apSSID;
extern const char* apPassword;
extern const char* udpIP;
extern const int   udpPort;

// TIMING SETTINGS
extern const int serialBaud;
extern const int mainDelay;
extern const int duetCheckDelay;

// IMU ANGLES
extern float measPitch;

extern float refPitch;   // recorded at startup

extern float lastIMUPitch;
extern float lastIMUYaw;

extern Vector<3> gravityRef;
extern Vector<3> gravity;


// COMMANDED ANGLES
extern float currentComPitch;
extern float currentComYaw;
extern float previousComPitch;
extern float previousComYaw;
extern float errorPitch;
// extern float errorYaw;

// EXTRUDER TIP POSITION
extern float currentX;
extern float currentY;
extern float currentZ;

// DUET STATUS
extern char  status;
extern float fractionPrinted;

// ANGLE VERIFICATION SETTINGS
extern const float angleErrThres;
extern const float imuStableThres;
extern const int   stableDur;
extern const int   settleDur;

// STABILITY AND MOTION TRACKING
extern bool  isStable;
extern bool  newCommand;
extern unsigned long stableStartTime;
extern unsigned long lastDuetCheckTime;
extern unsigned long lastMoveTime;

// FUNCTION DECLARATIONS
void initializeIMU();
void calcEuler();

void initializeDuet();
bool testDuetConnection();
void getValuesDuet();
void sendDuetMessage(String message);
String parseValue(String data, String key, char delimiter);

void initializeWiFiStart();
void initializeWiFiConnect();
void receiveUDPData();
void sendUDPPacket(String packet);
void sendDataToUnity();
void sendErrorToUnity();
void sendSuccessToUnity();

void checkIMUSettled();
void resumePrint();
bool verifyAngle();