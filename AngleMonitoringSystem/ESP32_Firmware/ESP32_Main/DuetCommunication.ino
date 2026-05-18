/*
 * Duet Communication Functions
 * Supplementary file for ESP32_One.ino
 *
 * FUNCTIONS:
 * initializeDuet()     - Initialize Serial2 connection to Duet
 * testDuetConnection() - Test if Duet is responding (sends M115)
 * getValuesDuet()      - Poll M114 for position and M408 for status
 * parseValue()         - Extract value from delimited response string
 * sendDuetMessage()    - Send M291 popup message to Duet Web Control
 */

 #include "globals.h"

/**
 * Initialize Serial2 connection to Duet (GPIO16/17)
 */
void initializeDuet() {
  Serial.print("Initializing Duet serial connection...\n");
  pinMode(LED_PIN, OUTPUT);

  duetSerial.begin(115200, SERIAL_8N1, 16, 17);
  delay(500);

  bool connect = testDuetConnection();
    
  // Uncomment to block until Duet responds
  // if (!connect) { Serial.print("WARNING: Duet connection failed. Check wiring."); }
  // while (!connect) {
  //   Serial.print(".");
  //   digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  //   delay(200);
  //   connect = testDuetConnection();
  // }

  digitalWrite(LED_PIN, HIGH);
  Serial.print("\nDuet connected!\n");
}


/**
 * Test Duet connection (sends M115, waits 1 second for response)
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
 * Poll Duet for position (M114) and status (M408)
 */
void getValuesDuet() {
  // GET POSITION (M114)
  while (duetSerial.available()) duetSerial.read();

  duetSerial.println("M114");
  delay(50);

  if (duetSerial.available()) {
    String response = duetSerial.readStringUntil('\n');

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
 * Parse value from response string
 * Works for M114 ("X:150.5") and JSON ("status":"I")
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
 * Send M291 message to Duet Web Control
 */
void sendDuetMessage(String message) {
  String gcode = "M291 P\"" + message + "\" S1\n";
  duetSerial.println(gcode);
}
