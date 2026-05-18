/*
 * Duet Communication Functions
 * Supplementary file for ESP32_One.ino
 * For ESP32 #1 (stationary)
 *
 * FUNCTIONS:
 * initializeDuet()          - Initializes Serial2 and blocks until Duet responds
 * testDuetConnection()      - Sends M115 and returns true if Duet firmware responds
 * getValuesDuet()           - Polls M114 for XYZ/AB position and M408 for print status
 * parseValue()              - Extracts a value from a delimited response string
 * sendDuetMessage()         - Sends an M291 popup message to Duet Web Control
 */

 #include "globals.h"

/**
 * Initialize serial connection to Duet
 * Called once during setup
 */
void initializeDuet() {
  Serial.print("Initializing Duet serial connection...\n");
  pinMode(LED_PIN, OUTPUT);

  duetSerial.begin(115200, SERIAL_8N1, 16, 17);
  delay(500);

  bool connect = testDuetConnection();
    
  // if (!connect) { Serial.print("WARNING: Duet connection failed. Check wiring."); }
  // while (!connect) {
  //   Serial.print(".");

  //   // blink while waiting
  //   digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  //   delay(200);

  //   connect = testDuetConnection();
  // }

  digitalWrite(LED_PIN, HIGH);  // solid on when connected
  Serial.print("\nDuet connected!\n");
}


/**
 * Test if Duet is responding over serial
 * Sends M115 and waits up to 1 second for response
 * Returns true if Duet responds
 */
bool testDuetConnection() {
  while (duetSerial.available()) duetSerial.read();

  duetSerial.println("M115");

  unsigned long startTime = millis();
  while (millis() - startTime < 1000) {
    if (duetSerial.available()) {
      String response = duetSerial.readStringUntil('\n');
      if (response.indexOf("FIRMWARE") >= 0 || response.indexOf("ok") >= 0) {
        return true;
      }
    }
    delay(10);
  }
  return false;
}


/**
 * Request current position, angles, status, and fraction printed from Duet
 * Sends M114 for position and M408 for status
 * Updates global variables
 */
void getValuesDuet() {
  // GET POSITION (M114)
  while (duetSerial.available()) duetSerial.read();

  duetSerial.println("M114");
  delay(50);

  if (duetSerial.available()) {
    String response = duetSerial.readStringUntil('\n');
    // Serial.print(response);
    // Serial.print("\n");

    currentX        = parseValue(response, "X:", ' ').toFloat();
    currentY        = parseValue(response, "Y:", ' ').toFloat();
    currentZ        = parseValue(response, "Z:", ' ').toFloat();
    currentComYaw   = parseValue(response, "A:", ' ').toFloat();

    currentComPitch = parseValue(response, "B:", ' ').toFloat();
  }

  // GET STATUS (M408)
  while (duetSerial.available()) duetSerial.read();

  duetSerial.println("M408 S0");
  delay(100);

  String statusResponse = "";
  while (duetSerial.available()) {
    statusResponse += (char)duetSerial.read();
  }

  if (statusResponse.length() > 0) {
    String statusStr = parseValue(statusResponse, "\"status\":\"", '"');
    status = statusStr[0];

    fractionPrinted = parseValue(statusResponse, "\"fraction_printed\":", ',').toFloat();
  }
}


/**
 * Parse a value from a response string given a key and delimiter
 * Works for M114 style "X:150.5" and JSON style "status":"I"
 * Returns value as String, call .toFloat() if needed
 */
String parseValue(String data, String key, char delimiter) {
  int startIndex = data.indexOf(key);
  if (startIndex == -1) return "";

  startIndex += key.length();
  int endIndex = data.indexOf(delimiter, startIndex);
  if (endIndex == -1) endIndex = data.length();

  return data.substring(startIndex, endIndex);
}


/**
 * Display a message on Duet Web Control
 * Uses M291 non-blocking popup (S1)
 */
void sendDuetMessage(String message) {
  String gcode = "M291 P\"" + message + "\" S1\n";
  duetSerial.println(gcode);
}
