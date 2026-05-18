/*
 * Angle Verification Functions
 * Supplementary file for ESP32_One.ino
 * For ESP32 #1 (stationary
 *
 * FUNCTIONS:
 * pausePrint()      - Sends M25 pause command to Duet and notifies Unity
 * resumePrint()     - Sends M24 resume command to Duet and notifies Unity
 * verifyAngle()     - Compares measPitch vs currentComPitch, returns true if within threshold
 * checkIMUSettled() - Monitors pitch change each loop to determine if plate has stopped moving
 */

 #include "globals.h"


/**
 * Send pause command to Duet
 */
void pausePrint() {
  Serial.print("SENDING PAUSE COMMAND TO DUET\n");
  sendDuetMessage("PAUSING PRINT!");
  duetSerial.println("M25");
  delay(100);
}


/**
 * Send resume command to Duet
 */
void resumePrint() {
  Serial.print("SENDING RESUME COMMAND TO DUET\n");
  sendDuetMessage("PRINT IS BEING RESUMED!");
  duetSerial.println("M24");
  delay(100);
}

/**
 * Compare measured IMU angles against commanded Duet angles
 * Sends error or success notification to Duet and Unity
 */
bool verifyAngle() {
  Serial.print("\nVERIFYING ANGLE\n");

  errorPitch = abs(measPitch - currentComPitch);
  // errorYaw = abs(measYaw - currentComYaw);
  // errorYaw = 0;

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
 * Check if IMU has stabilized after a move
 * Sets isStable = true once angles stop changing for stableDur ms
 */
void checkIMUSettled() {
  float pitchChange = abs(measPitch - lastIMUPitch);
  // float yawChange   = abs(measYaw - lastIMUYaw);

  bool moving = pitchChange >= imuStableThres;

  lastIMUPitch = measPitch;
  // lastIMUYaw   = measYaw;

  if (moving) {
    isStable = false;
    return;
  } else {
    isStable = true;
  }

}
