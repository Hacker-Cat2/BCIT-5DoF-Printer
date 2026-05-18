// IMU FUNCTIONS
// Handles BNO055 initialization and angle reading

#include "globals.h"

 /*
 * Initialize BNO055 IMU sensor
 * Called once during setup
 */
void initializeIMU() {
  Wire.begin(21, 22); // SDA, SCL pins
  if (!bno.begin()) {
    Serial.print("ERROR: BNO055 not detected. Check wiring or I2C address!\n");
    while (1);
  }
  delay(1000);
  bno.setExtCrystalUse(true);

  bno.setMode(Adafruit_BNO055::OPERATION_MODE_IMUPLUS); // gyro + accel only, no magnetometer
  
  // Wait 10s for sensor to stabilize
  Serial.print("Initializing BNO055...");
  unsigned long start = millis();
  while ((millis() - start) < 10000) {
      Serial.print(".");
      delay(1000);
  }
  Serial.print("\n");

  gravityRef = imu::Vector<3>(0.48, -0.10, 9.79); // calibrated reference at 0° pitch

  //gravityRef = bno.getVector(Adafruit_BNO055::VECTOR_GRAVITY); // uncomment to recalibrate

  Serial.print("Gravity Reference X: "); Serial.println(gravityRef.x());
  Serial.print("Gravity Reference Y: "); Serial.println(gravityRef.y());
  Serial.print("Gravity Reference Z: "); Serial.println(gravityRef.z());

  Serial.print("BNO055 initialized!\n");
}




/**
 * Calculate pitch angle from IMU gravity vector
 * Updates global measPitch in degrees
 */
void calcEuler() {

  gravity = bno.getVector(Adafruit_BNO055::VECTOR_GRAVITY);
  if (gravity.x() == 0 && gravity.y() == 0 && gravity.z() == 0) return; // skip if no data

  // Magnitude of current and reference gravity vectors
  float gravityMag    = sqrt(gravity.x()*gravity.x() + gravity.y()*gravity.y() + gravity.z()*gravity.z());
  float gravityRefMag = sqrt(gravityRef.x()*gravityRef.x() + gravityRef.y()*gravityRef.y() + gravityRef.z()*gravityRef.z());

  // Dot product between current and reference gravity vectors
  float dotGravity = ( gravity.x() * gravityRef.x() ) + ( gravity.y() * gravityRef.y() ) + ( gravity.z() * gravityRef.z() );

  // Angle between vectors = pitch relative to calibrated 0°
  float denom = gravityMag * gravityRefMag;
  if (denom < 0.001) return; // avoid dividing by zero
  
  float cosAngle = constrain(dotGravity / denom, -1.0, 1.0);
  measPitch = acos(cosAngle) * (180.0 / PI);

}




