/*
 * Unity Communication Functions
 * Supplementary file for ESP32_Main.ino
 *
 * FUNCTIONS:
 * initializeWiFiStart()    - Initialize WiFi hotspot and UDP communication
 * initializeWiFiConnect()  - Connect to WiFi hotspot
 * sendUDPPacket()          - Send UDP packet to Unity
 * sendDataToUnity()        - Send data packet with angles, position, and status
 * sendErrorToUnity()       - Send error message (triggers red background)
 * sendSuccessToUnity()     - Send success message (triggers green background)
 */

 #include "globals.h"

/**
 * Initialize WiFi hotspot and UDP
 * ESP32 creates access point at 192.168.4.1
 */
void initializeWiFiStart() {
  Serial.print("Starting WiFi hotspot...\n");
  
  WiFi.softAP(apSSID, apPassword);
  
  Serial.print("Hotspot started! IP: ");
  Serial.print(WiFi.softAPIP().toString());
  Serial.print("\n");

  udp.begin(udpPort);
  Serial.print("UDP listening on port: ");
  Serial.print(udpPort);
  Serial.print("\n");
}

/**
 * Connect to ESP32 hotspot
 * Blocks until connected
 */
void initializeWiFiConnect() {
  Serial.println("Connecting to hotspot...");

  WiFi.begin(apSSID, apPassword);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("\nConnected! IP: ");
  Serial.print(WiFi.localIP().toString());
  Serial.print("\n");


  udp.begin(udpPort);

  Serial.print("UDP listening on port: ");
  Serial.print(String(udpPort));
  Serial.print("\n");

}

/**
 * Receive UDP packets from Unity (pause/resume commands)
 */
void receiveUDPData() {
  int packetSize = udp.parsePacket();
  if (!packetSize) return;

  char buffer[255] = {0};
  int len = udp.read(buffer, packetSize);
  buffer[len] = '\0';
  String data = String(buffer);
  Serial.print(data);

  if (data.startsWith("IMU")) {
    measPitch = parseValue(data, "MP:", ',').toFloat();

    Serial.print("Pitch:");
    Serial.print(measPitch);
    Serial.print("\n");

  } else if (data.startsWith("PAUSE")) {
    if(status != 'A' || status != 'D' || status != 'I') {
      Serial.print("Pausing print\n");
      pausePrint();
    }
    
  } else if (data.startsWith("RESUME")) {
    if(status != 'R' || status != 'P') {
      Serial.print("Resuming\n");
      resumePrint();
    }
    
  } else {  
    Serial.print("Received Nothing :(\n");
  }
}

/**
 * Send UDP packet to Unity
 */
void sendUDPPacket(String packet) {
  udp.beginPacket(udpIP, udpPort);
  udp.print(packet);
  udp.endPacket();
}

/**
 * Send data packet to Unity (called every 100ms)
 * Format: "DATA,CP:xx,MP:xx,CY:xx,X:xx,Y:xx,Z:xx,FP:xx,S:x,EP:xx"
 */
void sendDataToUnity() {
  
  String packet = "DATA,";
  packet += "CP:" + String(currentComPitch, 2) + ",";
  packet += "MP:" + String(measPitch, 2) + ",";
  packet += "CY:" + String(currentComYaw, 2) + ",";
  packet += "X:"  + String(currentX, 2) + ",";
  packet += "Y:"  + String(currentY, 2) + ",";
  packet += "Z:"  + String(currentZ, 2) + ",";
  packet += "FP:" + String(fractionPrinted, 2) + ",";
  packet += "S:" + String(status, 2) + ",";
  packet += "EP:" + String(errorPitch);

  sendUDPPacket(packet);
}

/**
 * Send error message to Unity
 */
void sendErrorToUnity() {
  sendUDPPacket("ERROR");
  Serial.print("Error message sent to Unity\n");
}

/**
 * Send success message to Unity
 */
void sendSuccessToUnity() {
  sendUDPPacket("GOOD");
  Serial.print("Success message sent to Unity\n");
}
