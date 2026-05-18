/*
 * ESP32 Angle Verification System for Non-Planar 3D Printer
 * For ESP32 #1 (stationary) - Communication hub between ESP32 #2, Unity, and Duet 2
 * Main file - Configuration, setup, and main loop
 *
 * Author: Lyka Margareth Sabate
 * Team: Molten Motion (Kassia Ferguson, Keaton Westcott, Margareth Sabate)
 * Date: 2026
 *
 * OVERVIEW:
 * This ESP32 is stationary and acts as the central communication hub.
 * It hosts a WiFi hotspot that ESP32 #2 (on the rotating plate) connects to,
 * receives IMU angle data from ESP32 #2 via UDP, and forwards it to Unity
 * for visualization. It also communicates with the Duet 2 MCU via UART
 * to monitor print status, current position, and send pause/resume commands
 *
 * COMMUNICATION:
 * - ESP32 #2  →  ESP32 #1 : UDP over hosted WiFi hotspot (angle data)
 * - ESP32 #1  →  Unity    : UDP packet with pitch, yaw, status, and error
 * - ESP32 #1  ↔  Duet 2   : Serial2 at 115200 baud (G-code / status polling)
 */

#include "globals.h"

WiFiUDP udp;

Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x29);
sensors_event_t event; 

const char* apSSID     = "hehe";
const char* apPassword = "12345678";
const char* udpIP      = "192.168.4.255";
const int   udpPort    = 1234;

const int serialBaud     = 115200;
const int mainDelay      = 100;
const int duetCheckDelay = 200;

float measPitch    = 0;
float refPitch = 0; // recorded at startup

float lastIMUPitch = 0;
float lastIMUYaw   = 0;

Vector<3> gravityRef = {0,0,0};
Vector<3> gravity = {0,0,0};

float currentComPitch  = 0;
float previousComPitch = 0;

float currentComYaw = 0;

float errorPitch = 0;

float currentX = 0;
float currentY = 0;
float currentZ = 0;

char  status          = 0;
float fractionPrinted = 0;

const float angleErrThres  = 3.0;
const float imuStableThres = 0.001;
const int   stableDur      = 500;
const int   settleDur      = 500;

bool  isStable             = false;
bool  newCommand          = false;
unsigned long stableTime   = 5000;
unsigned long lastDuetCheckTime = 0;
unsigned long lastMoveTime      = 0;



void setup() {
  Serial.begin(serialBaud);
  Serial.print("ESP32 #2 - Communication to Unity\n");

  initializeIMU();  
  initializeDuet();
  initializeWiFiStart();

  Serial.print("System Ready!\n");
}


void loop() {
  // Recieve IMU Data from ESP32 #1
  // receiveUDPData();

  calcEuler();
  Serial.print("\tPitch ");
  Serial.print(measPitch, 4);
  Serial.print("\n");

  errorPitch = abs(measPitch - currentComPitch);

  // POLL DUET FOR POSITION, ANGLES, AND STATUS
  if ((millis() - lastDuetCheckTime) > duetCheckDelay) {
    getValuesDuet();
    lastDuetCheckTime = millis();
  }

  // CHECK IF IMU HAS SETTLED
  // checkIMUSettled();

  // DETECT IF COMMANDED ANGLES ARE STILL CHANGING
  bool pitchChanging = currentComPitch != previousComPitch;
  // bool yawChanging   = abs(currentComYaw   - previousComYaw)   > 0.1;
  // bool yawChanging = 0;

  if (pitchChanging) {
    lastMoveTime     = millis();

    previousComPitch = currentComPitch;
  }

  float totalTime = millis() - lastMoveTime;

  // Verify after a certain amount of time the angle hasnt changed and IMU and Duet are settled
  if (newCommand && (totalTime > stableTime) ) {
    newCommand = false;
    verifyAngle();
  }

  // SEND DATA TO UNITY
  sendDataToUnity();

  delay(mainDelay);
}
