/*
 * Angle Verification Functions
 * Supplementary file for ESP32_Main.ino
 *
 * FUNCTIONS:
 * pausePrint()      - Send M25 pause command to Duet
 * resumePrint()     - Send M24 resume command to Duet
 * verifyAngle()     - Compare measured vs commanded pitch, notify if error > 3°
 * checkIMUSettled() - Monitor pitch change to determine if plate has stopped moving
 */
 
 #include "globals.h"

/**
 * Send M25 pause command to Duet
 */
void pausePrint() {
  Serial.print("SENDING PAUSE COMMAND TO DUET\n");
  sendDuetMessage("PAUSING PRINT!");
  duetSerial.println("M25");
  delay(100);
}


/**
 * Send M24 resume command to Duet
 */
void resumePrint() {
  Serial.print("SENDING RESUME COMMAND TO DUET\n");
  sendDuetMessage("PRINT IS BEING RESUMED!");
  duetSerial.println("M24");
  delay(100);
}

/**
 * Compare measured vs commanded pitch
 * Returns false if error exceeds 3° threshold
 */
bool verifyAngle() {
  Serial.print("\nVERIFYING ANGLE\n");

  errorPitch = abs(measPitch - currentComPitch);

  if (errorPitch > angleErrThres) {
    Serial.print("ERROR: Pitch mismatch! Error: ");
    Serial.print(errorPitch, 2);
    Serial.print(" deg, Threshold: ");
    Serial.print(angleErrThres, 2);
    Serial.print("\n");

    sendDuetMessage("ANGLE ERROR - Check pitch axis!");
    sendErrorToUnity();

    return false;

  } else {
    Serial.print("Angle verified!\n");
    sendSuccessToUnity();

    return true;
  }
}

/**
 * Check if IMU has stabilized after movement
 */
void checkIMUSettled() {
  float pitchChange = abs(measPitch - lastIMUPitch);

  bool moving = pitchChange >= imuStableThres;

  lastIMUPitch = measPitch;

  if (moving) {
    isStable = false;
    return;
  } else {
    isStable = true;
  }
}
