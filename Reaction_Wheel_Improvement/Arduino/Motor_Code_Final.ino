//-------------------------------------------------------------------------//
//-------- Code for CubeSat Reaction Wheel - Nimbus Project ---------------//
//------------ Creat by Yann HUGUET - Date : 27/11/2023     ---------------//
//-------------------------------------------------------------------------//

#include <MPU6050_tockn.h>
#include <Wire.h>
#include <Arduino.h>
#include <Adafruit_Sensor.h>

MPU6050 mpu6050(Wire);

long timer = 0;
int i = 0;
const int Mot1P = 8; // Arduino pin connected to EN1 of L298N
const int Mot1M = 9; // Arduino pin connected to ENA of L298N
const int Mot2P = 7; // Arduino pin connected to EN1 of L298N
const int Mot2M = 6; // Arduino pin connected to ENA of L298N
const int Ena1 = 10;
const int Ena2 = 5;

const int MPU = 0x68; // MPU6050 I2C address
float AccX, AccY, AccZ;
float GyroX, GyroY, GyroZ;
float accAngleX, accAngleY, gyroAngleX, gyroAngleY, gyroAngleZ;
float roll, pitch, yaw;
float AccErrorX, AccErrorY, GyroErrorX, GyroErrorY, GyroErrorZ;
float elapsedTime, currentTime, previousTime;
int c = 0;

unsigned long previousYawInversion = 0; // Variable to store the time of the last yaw inversion
const unsigned long inversionInterval = 760; // Inversion interval in milliseconds (1 second = 1000 milliseconds)

//------------------init to correct the drifting values----------------------

double previousYaw = 0.0;
double previousPitch = 0.0;
double previousRoll = 0.0;
double gyroThreshold = 0.5;


// Setting up the configurations and initializing necessary components
void setup() {
  Serial.begin(9600);                     // Serial Monitor for testing
  Serial.begin(19200);
  Serial1.begin(115200);                  // Pi UART faster speed

  pinMode(Mot1P, OUTPUT);
  pinMode(Mot1M, OUTPUT);
  pinMode(Mot2P, OUTPUT);
  pinMode(Mot2M, OUTPUT);
  pinMode(Ena1, OUTPUT);
  pinMode(Ena2, OUTPUT);


  Wire.begin();                      // Initialize communication
  Wire.beginTransmission(MPU);       // Start communication with MPU6050 // MPU=0x68
  Wire.write(0x6B);                  // Talk to the register 6B
  Wire.write(0x00);                  // Make reset - place a 0 into the 6B register
  Wire.endTransmission(true);        // End the transmission
  // Configure Accelerometer Sensitivity - Full Scale Range (default +/- 2g)
  Wire.write(0x1C);                  // Talk to the ACCEL_CONFIG register (1C hex)
  Wire.write(0x10);                  // Set the register bits as 00010000 (+/- 8g full scale range)
  Wire.endTransmission(true);
  // Configure Gyro Sensitivity - Full Scale Range (default +/- 250deg/s)
  Wire.write(0x1B);                   // Talk to the GYRO_CONFIG register (1B hex)
  Wire.write(0x10);                   // Set the register bits as 00010000 (1000deg/s full scale)
  Wire.endTransmission(true);
  delay(20);
  // Call this function if you need to get the IMU error values for your module
  calculate_IMU_error();
  delay(20);
}

void loop() {

  // Read sensor data and perform calculations
  unsigned long currentMillis = millis(); // Current time in milliseconds
  Wire.beginTransmission(MPU);
  Wire.write(0x3B); // Start with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true); // Read 6 registers total, each axis value is stored in 2 registers

  // For a range of +-2g, we need to divide the raw values by 16384, according to the datasheet
  AccX = (Wire.read() << 8 | Wire.read()) / 16384.0; // X-axis value
  AccY = (Wire.read() << 8 | Wire.read()) / 16384.0; // Y-axis value
  AccZ = (Wire.read() << 8 | Wire.read()) / 16384.0; // Z-axis value

  // Calculating Roll and Pitch from the accelerometer data
  accAngleX = (atan(AccY / sqrt(pow(AccX, 2) + pow(AccZ, 2))) * 180 / PI) - 0.58; // AccErrorX ~(0.58) See the calculate_IMU_error()custom function for more details
  accAngleY = (atan(-1 * AccX / sqrt(pow(AccY, 2) + pow(AccZ, 2))) * 180 / PI) + 1.58; // AccErrorY ~(-1.58)

// === Read gyroscope data === //

  previousTime = currentTime; // Store previous time before reading current time
  currentTime = millis(); // Read current time
  elapsedTime = (currentTime - previousTime) / 1000; // Divide by 1000 to get seconds - better by 500 for the 3D view
  Wire.beginTransmission(MPU);
  Wire.write(0x43); // Gyro data first register address 0x43
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true); // Read 4 registers total, each axis value is stored in 2 registers
  GyroX = (Wire.read() << 8 | Wire.read()) / 131.0; // For a 250deg/s range we have to divide first the raw value by 131.0, according to the datasheet
  GyroY = (Wire.read() << 8 | Wire.read()) / 131.0;
  GyroZ = (Wire.read() << 8 | Wire.read()) / 131.0;

  // Correct the outputs with the calculated error values
  GyroX = GyroX + 0.56; // GyroErrorX ~(-0.56)
  GyroY = GyroY - 2; // GyroErrorY ~(2)
  GyroZ = GyroZ + 0.79; // GyroErrorZ ~ (-0.8)

  double gyroChange = abs(GyroX) + abs(GyroY);

  // Update angles based on gyroscopes only if the change exceeds the threshold
  if (gyroChange > gyroThreshold) {
    pitch = pitch + GyroY * elapsedTime;
    roll = roll + GyroX * elapsedTime;
  }
  yaw =  yaw + GyroZ * elapsedTime;

  // Correct angles to prevent accumulation
  pitch = pitch - previousPitch;
  roll = roll - previousRoll;

  // Update previous values for the next iteration
  previousPitch = GyroY * elapsedTime;
  previousRoll = GyroX * elapsedTime;

  // Complementary filter - combine acceleromter and gyro angle values
  roll = (0.96 * roll + 0.04 * accAngleX);
  pitch = (0.96 * pitch + 0.04 * accAngleY);

  // Correct yaw drift by incrementing at regular intervals
  if (currentMillis - previousYawInversion >= inversionInterval) {
    yaw = yaw + 1; // Correct yaw drift by adding 1 every regular interval
    previousYawInversion = currentMillis;
  }

// Check if yaw angle has completed a full rotation and reset it to 0 after each full rotation
if (yaw < -360 || yaw > 360) {
  yaw = 0;
}

// Print the roll, pitch, and yaw values on the serial monitor
Serial.print(roll);
Serial.print("/");
Serial.print(pitch);
Serial.print("/");
Serial.println(yaw);

//----- Control Motor 2 based on roll angle  ----//
if (roll < -10) {
  digitalWrite(Mot2P, HIGH);
  digitalWrite(Mot2M, LOW);
  analogWrite(Ena1, 255);
  analogWrite(Ena2, 255);
  
} else if (roll > 10) {
  digitalWrite(Mot2P, LOW);
  digitalWrite(Mot2M, HIGH);
  analogWrite(Ena1, 255);
  analogWrite(Ena2, 255);
  
} else if (roll > -10 && roll < 10) {
  digitalWrite(Mot2P, LOW);
  digitalWrite(Mot2M, LOW);
  analogWrite(Ena1, 255);
  analogWrite(Ena2, 255);
}

//----- Control Motor 1 based on yaw angle ----//
if (yaw < -10) {
  digitalWrite(Mot1P, HIGH);
  digitalWrite(Mot1M, LOW);
  analogWrite(Ena1, 255);
  analogWrite(Ena2, 255);
  
} else if (yaw > 10) {
  digitalWrite(Mot1P, LOW);
  digitalWrite(Mot1M, HIGH);
  analogWrite(Ena1, 255);
  analogWrite(Ena2, 255);
  
} else if (yaw > -10 && yaw < 10) {
  digitalWrite(Mot1P, LOW);
  digitalWrite(Mot1M, LOW);
}

}
//-----------------------calculate_IMU_error for 3D----------------------------
void calculate_IMU_error() {
  // This function calculates the accelerometer and gyro data errors.
  // It reads sensor values multiple times to calculate the average error values.

  // Read accelerometer values 200 times to calculate error values
  while (c < 200) {
    Wire.beginTransmission(MPU);
    Wire.write(0x3B); // Start with register 0x3B (ACCEL_XOUT_H)
    Wire.endTransmission(false);
    Wire.requestFrom(MPU, 6, true);

    // Read accelerometer data for each axis and accumulate readings
    AccX = (Wire.read() << 8 | Wire.read()) / 16384.0 ;
    AccY = (Wire.read() << 8 | Wire.read()) / 16384.0 ;
    AccZ = (Wire.read() << 8 | Wire.read()) / 16384.0 ;

    // Calculate error by summing the arctangent of certain accelerometer ratios
    AccErrorX = AccErrorX + ((atan((AccY) / sqrt(pow((AccX), 2) + pow((AccZ), 2))) * 180 / PI));
    AccErrorY = AccErrorY + ((atan(-1 * (AccX) / sqrt(pow((AccY), 2) + pow((AccZ), 2))) * 180 / PI));
    c++;
  }

  // Calculate average error values for accelerometer
  AccErrorX = AccErrorX / 200;
  AccErrorY = AccErrorY / 200;
  c = 0;

  // Read gyro values 200 times to calculate error values
  while (c < 200) {
    Wire.beginTransmission(MPU);
    Wire.write(0x43); // Gyro data first register address 0x43
    Wire.endTransmission(false);
    Wire.requestFrom(MPU, 6, true);

    // Read gyro data for each axis and accumulate readings
    GyroX = Wire.read() << 8 | Wire.read();
    GyroY = Wire.read() << 8 | Wire.read();
    GyroZ = Wire.read() << 8 | Wire.read();

    // Calculate error by summing gyro readings
    GyroErrorX = GyroErrorX + (GyroX / 131.0);
    GyroErrorY = GyroErrorY + (GyroY / 131.0);
    GyroErrorZ = GyroErrorZ + (GyroZ / 131.0);
    c++;
  }

  // Calculate average error values for gyro
  GyroErrorX = GyroErrorX / 200;
  GyroErrorY = GyroErrorY / 200;
  GyroErrorZ = GyroErrorZ / 200;

  // Print the calculated error values on the Serial Monitor for calibration
  Serial.print("AccErrorX: ");
  Serial.println(AccErrorX);
  Serial.print("AccErrorY: ");
  Serial.println(AccErrorY);
  Serial.print("GyroErrorX: ");
  Serial.println(GyroErrorX);
  Serial.print("GyroErrorY: ");
  Serial.println(GyroErrorY);
  Serial.print("GyroErrorZ: ");
  Serial.println(GyroErrorZ);
}
