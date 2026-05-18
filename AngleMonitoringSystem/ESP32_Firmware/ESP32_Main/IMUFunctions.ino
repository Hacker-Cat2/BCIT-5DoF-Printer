/*
 * IMU Functions
 * Supplementary file for ESP32_One.ino
 *
 * FUNCTIONS:
 * initializeIMU() - Initialize BNO055 in IMUPLUS mode with hard-coded calibration
 * calcEuler()     - Calculate pitch angle using gravity vector dot product method
 */

#include "globals.h"

/**
 * Initialize BNO055 IMU (I2C at GPIO21/22)
 * IMUPLUS mode: accelerometer + gyroscope only (no magnetometer)
 * Hard-coded gravity reference at 0° pitch
 */
void initializeIMU() {
  Wire.begin(21, 22);
  if (!bno.begin()) {
    Serial.print("ERROR: BNO055 not detected. Check wiring or I2C address!\n");
    while (1);
  }
  delay(1000);
  bno.setExtCrystalUse(true);

  bno.setMode(Adafruit_BNO055::OPERATION_MODE_IMUPLUS);
  
  // 10-second stabilization period
  Serial.print("Initializing BNO055...");
  unsigned long start = millis();
  while ((millis() - start) < 10000) {
      Serial.print(".");
      delay(1000);
  }
  Serial.print("\n");

  // Hard-coded gravity vector (measured when plate is flat)
  gravityRef = imu::Vector<3>(0.48, -0.10, 9.79);

  // Uncomment to recalibrate
  //gravityRef = bno.getVector(Adafruit_BNO055::VECTOR_GRAVITY);

  Serial.print("Gravity Reference X: "); Serial.println(gravityRef.x());
  Serial.print("Gravity Reference Y: "); Serial.println(gravityRef.y());
  Serial.print("Gravity Reference Z: "); Serial.println(gravityRef.z());

  Serial.print("BNO055 initialized!\n");
}


/**
 * Calculate pitch angle using gravity vector dot product
 * Updates measPitch in degrees
 */
void calcEuler() {

  gravity = bno.getVector(Adafruit_BNO055::VECTOR_GRAVITY);
  if (gravity.x() == 0 && gravity.y() == 0 && gravity.z() == 0) return;

  // Magnitude of gravity vectors
  float gravityMag    = sqrt(gravity.x()*gravity.x() + gravity.y()*gravity.y() + gravity.z()*gravity.z());
  float gravityRefMag = sqrt(gravityRef.x()*gravityRef.x() + gravityRef.y()*gravityRef.y() + gravityRef.z()*gravityRef.z());

  // Dot product
  float dotGravity = (gravity.x() * gravityRef.x()) + (gravity.y() * gravityRef.y()) + (gravity.z() * gravityRef.z());

  // Calculate angle between vectors
  float denom = gravityMag * gravityRefMag;
  if (denom < 0.001) return;
  
  float cosAngle = constrain(dotGravity / denom, -1.0, 1.0);
  measPitch = acos(cosAngle) * (180.0 / PI);
}
