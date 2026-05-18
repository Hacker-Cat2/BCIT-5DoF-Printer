/*
 * Unity Communication Functions
 * Handles WiFi hotspot and UDP data transmission to othe esp
 *
 * Packet types:
 * DATA  - sends all data (angles, quaternion, XYZ position)
 * ERROR - sends when angle is off (cmdPitch, measPitch, error)
 * OK    - sends when angle is good (cmdPitch, measPitch, error)
 */

 #include "globals.h"


// WIFI INITIALIZATION
/**
 * Initialize WiFi hotspot and UDP communication
 * ESP32 creates its own WiFi network that Unity PC connects to
 * Called once during setup
 */
void initializeWiFiStart() {
  Serial.print("Starting WiFi hotspot...\n");
  
  // Create WiFi access point (hotspot)
  // Devices can connect to this network
  WiFi.softAP(apSSID, apPassword);
  
  Serial.print("Hotspot started! IP: "); //Default IP is 192.168.4.1
  Serial.print(WiFi.softAPIP().toString()); //Default IP is 192.168.4.1
  Serial.print("\n"); //Default IP is 192.168.4.1

  
  // Start listening on UDP port
  udp.begin(udpPort);
  Serial.print("UDP listening on port: ");
  Serial.print(udpPort);
  Serial.print("\n");

}

// WIFI INITIALIZATION
/**
 * Connect to ESP32 #1 hotspot
 * Blocks until connected
 * Called once during setup
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


// ESP2 DATA COMMUNICATION
/**
 * Receive and parse UDP packet 
 */
void receiveUDPData() {
  int packetSize = udp.parsePacket();
  if (!packetSize) return; // no packet available

  char buffer[255] = {0};          // zero-initialize
  int len = udp.read(buffer, packetSize);
  buffer[len] = '\0';              // null terminate
  String data = String(buffer);
  Serial.print(data);

  // only process IMU packets
  if (data.startsWith("IMU")) {
    measPitch = parseValue(data, "MP:", ',').toFloat();

    Serial.print("Pitch:");
    Serial.print(measPitch);
    Serial.print("\n");

  } else if (data.startsWith( "PAUSE" )) {
    if( status != 'A' || status != 'D' || status != 'I') {
      Serial.print("Pausing print\n");
      pausePrint();
    }
    
  } else if ( data.startsWith("RESUME") ) {

    if( status != 'R' || status != 'P' ) {
      Serial.print("Resuming\n");
      resumePrint();
    }
    
  } else {  
    Serial.print("Received Nothing :(\n");
  }

  
}


// UNITY DATA COMMUNICATION
/**
 * Send a UDP packet to Unity
 * Handles addressing, writing, and sending in one call
 */
void sendUDPPacket(String packet) {
  udp.beginPacket(udpIP, udpPort); // sets where the packet is going
  udp.print(packet); // writing the packet
  udp.endPacket(); // sending and finishing the packet
}

/**
 * Sends and created data packet to Unity for visualization
 * Called every 100ms in main loop
 * Data Packet Format: "DATA,CP:0.0,MP:0.0,CY:0.0,X:0.0,Y:0.0,Z:0.0,FP:0.0,S:'I',EP:0.0"
 */
void sendDataToUnity() {
  
  String packet = "DATA,";
  packet += "CP:" + String(currentComPitch, 2) + ",";   // commanded pitch (degrees)
  packet += "MP:" + String(measPitch, 2) + ",";         // measured pitch (degrees)
  packet += "CY:" + String(currentComYaw, 2) + ",";     // commanded yaw (degrees)
  packet += "X:"  + String(currentX, 2) + ",";          // X position (mm)
  packet += "Y:"  + String(currentY, 2) + ",";          // Y position (mm)
  packet += "Z:"  + String(currentZ, 2) + ",";          // Z position (mm)
  packet += "FP:" + String(fractionPrinted, 2) + ",";   // fraction printed (0.0 - 1.0)
  packet += "S:" + String(status, 2) + ",";             // printer status (A/D/I/R/P)
  packet += "EP:" + String(errorPitch);                 // pitch error (degrees)

  sendUDPPacket(packet);

  // Serial.print(packet);
  // Serial.print("\n");

}

/**
 * Sends error message to Unity
 * Called when angle verification fails
 * 
 * Packet format: "ERROR,<cmd_pitch>,<actual_pitch>,<error>"
 * Example: "ERROR,15.00,12.50,2.50"
 */
void sendErrorToUnity() {
  sendUDPPacket("ERROR");

  Serial.print("Error message sent to Unity\n");
}

/**
 * Sends success message to Unity
 * Called when angle verification passes
 * 
 * Packet format: "OK,<cmd_pitch>,<actual_pitch>,<error>"
 * Example: "OK,15.00,15.10,0.10"
 */
void sendSuccessToUnity() {

  sendUDPPacket("GOOD");

  Serial.print("Success message sent to Unity\n");
}