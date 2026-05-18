/*
 * ESP32 Angle Monitoring and Visualization System
 * Main file - Configuration, setup, and main loop
 *
 * Author: Lyka Margareth Sabate
 * Team: Molten Motion (Kassia Ferguson, Keaton, Margareth Sabate)
 * Date: May 17, 2026
 *
 * OVERVIEW:
 * ESP32 reads pitch angle from BNO055 IMU (mounted on rotating pitch mechanism)
 * via I2C. Communicates with Duet 2 controller via UART to
 * monitor print status and position. Sends real-time data to Unity visualization
 * via WiFi UDP for operator monitoring.
 *
 * COMMUNICATION:
 * - BNO055  ↔  ESP32 : I2C at 100 kHz
 * - ESP32   ↔  Duet  : UART at 115200 baud (GCode commands)
 * - ESP32   →  Unity : UDP over WiFi (real-time visualization)
 */

#include "globals.h"

WiFiUDP udp;

Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x29);
sensors_event_t event; 

// WIFI SETTINGS
const char* apSSID     = "hehe";
const char* apPassword = "12345678";
const char* udpIP      = "192.168.4.255";
const int   udpPort    = 1234;

// TIMING SETTINGS
const int serialBaud     = 115200;
const int mainDelay      = 100;
const int duetCheckDelay = 200;

// IMU ANGLES
float measPitch    = 0;
float refPitch     = 0;
float lastIMUPitch = 0;
float lastIMUYaw   = 0;

Vector<3> gravityRef = {0,0,0};
Vector<3> gravity    = {0,0,0};

// COMMANDED ANGLES
float currentComPitch  = 0;
float previousComPitch = 0;
float currentComYaw    = 0;
float errorPitch       = 0;

// EXTRUDER POSITION
float currentX = 0;
float currentY = 0;
float currentZ = 0;

// DUET STATUS
char  status          = 0;
float fractionPrinted = 0;

// ANGLE VERIFICATION SETTINGS
const float angleErrThres  = 3.0;
const float imuStableThres = 0.001;
const int   stableDur      = 500;
const int   settleDur      = 500;

// STATE FLAGS
bool isStable  = false;
bool newCommand = false;

// TIMING TRACKING
unsigned long stableTime        = 5000;
unsigned long lastDuetCheckTime = 0;
unsigned long lastMoveTime      = 0;



void setup() {
  Serial.begin(serialBaud);
  Serial.print("ESP32 Angle Monitoring System\n");

  initializeIMU();  
  initializeDuet();
  initializeWiFiStart();

  Serial.print("System Ready!\n");
}


void loop() {
  // Calculate pitch angle from IMU
  calcEuler();
  Serial.print("\tPitch ");
  Serial.print(measPitch, 4);
  Serial.print("\n");

  errorPitch = abs(measPitch - currentComPitch);

  // Poll Duet every 200ms
  if ((millis() - lastDuetCheckTime) > duetCheckDelay) {
    getValuesDuet();
    lastDuetCheckTime = millis();
  }

  // Detect movement
  bool pitchChanging = currentComPitch != previousComPitch;

  if (pitchChanging) {
    lastMoveTime     = millis();
    previousComPitch = currentComPitch;
  }

  float totalTime = millis() - lastMoveTime;

  // Verify after settling period
  if (newCommand && (totalTime > stableTime)) {
    newCommand = false;
    verifyAngle();
  }

  // Send data to Unity
  sendDataToUnity();

  delay(mainDelay);
}
