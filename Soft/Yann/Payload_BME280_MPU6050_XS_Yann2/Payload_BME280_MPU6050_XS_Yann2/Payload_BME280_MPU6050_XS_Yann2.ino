 //include all the library

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <MPU6050_tockn.h>
#include <EEPROM.h>

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme;
MPU6050 mpu6050(Wire);


//-----------------------------------------------------------------------------

//initialisation of the diferent pin and value

long timer = 0;
int bmePresent;
int RXLED = 17;  // The RX LED has a defined Arduino pin
int greenLED = 9;
int blueLED = 8;
int Sensor1 = 0;
float Sensor2 = 0;
void eeprom_word_write(int addr, int val);
short eeprom_word_read(int addr);
int first_time = true;
int first_read = true;
float T2 = 26.3;                          // Temperature data point 1
float R2 = 167;                           // Reading data point 1
float T1 = 2;                             // Temperature data point 2
float R1 = 179;                           // Reading data point 2
int sensorValue;
float Temp;
float rest;

//....

const int MPU = 0x68;
float AccX, AccY, AccZ;
float GyroX, GyroY, GyroZ;
float accAngleX, accAngleY, gyroAngleX, gyroAngleY, gyroAngleZ;
float roll, pitch, yaw;
float AccErrorX, AccErrorY, GyroErrorX, GyroErrorY, GyroErrorZ;
float elapsedTime, currentTime, previousTime;
int c = 0;

//....

void setup() {

  Serial.begin(9600);                     // Serial Monitor for testing
  Serial.begin(19200);
  Serial1.begin(115200);                  // Pi UART faster speed

  Serial.println("Starting!");
//....
  Wire.begin();                      // Initialize comunication
  Wire.beginTransmission(MPU);       // Start communication with MPU6050 // MPU=0x68
  Wire.write(0x6B);                  // Talk to the register 6B
  Wire.write(0x00);                  // Make reset - place a 0 into the 6B register
  Wire.endTransmission(true);        //end the transmission

  // Configure Accelerometer Sensitivity - Full Scale Range (default +/- 2g)
  Wire.beginTransmission(MPU);
  Wire.write(0x1C);                  //Talk to the ACCEL_CONFIG register (1C hex)
  Wire.write(0x10);                  //Set the register bits as 00010000 (+/- 8g full scale range)
  Wire.endTransmission(true);
  // Configure Gyro Sensitivity - Full Scale Range (default +/- 250deg/s)
  Wire.beginTransmission(MPU);
  Wire.write(0x1B);                   // Talk to the GYRO_CONFIG register (1B hex)
  Wire.write(0x10);                   // Set the register bits as 00010000 (1000deg/s full scale)
  Wire.endTransmission(true);
  delay(20);
  // Call this function if you need to get the IMU error values for your module
  calculate_IMU_error();
  delay(20);
//.....

  blink_setup();

  blink(500);
  delay(250);
  blink(500);
  delay(250);
  led_set(greenLED, HIGH);
  delay(250);
  led_set(greenLED, LOW);
  led_set(blueLED, HIGH);
  delay(250);
  led_set(blueLED, LOW);

//-----------------------------------------------------------------------------

  if (bme.begin(0x76)) {
    bmePresent = 1;
  } else {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    bmePresent = 0;
  }

//-----------------------------------------------------------------------------

  mpu6050.begin();

  if (eeprom_word_read(0) == 0xA07)             //if you want to init the position of gyro change for 08 and televerse
  {
    Serial.println("Reading gyro offsets from EEPROM\n");

    float xOffset = ((float)eeprom_word_read(1)) / 100.0;
    float yOffset = ((float)eeprom_word_read(2)) / 100.0;
    float zOffset = ((float)eeprom_word_read(3)) / 100.0;

    //float xOffset = 0.0;
    //float yOffset = 0.0;
    //float zOffset = 0.0;

    Serial.println(xOffset, DEC);
    //Serial.println("/");
    Serial.println(yOffset, DEC);
    //Serial.println("/");
    Serial.println(zOffset, DEC);

//-----------------------------------------------------------------------------

    mpu6050.setGyroOffsets(xOffset, yOffset, zOffset);
  }
  else
  {
    Serial.println("Calculating gyro offsets and storing in EEPROM\n");

    mpu6050.calcGyroOffsets(true);

    eeprom_word_write(0, 0xA07);
    eeprom_word_write(1, (int)(mpu6050.getGyroXoffset() * 100.0) + 0.5);
    eeprom_word_write(2, (int)(mpu6050.getGyroYoffset() * 100.0) + 0.5);
    eeprom_word_write(3, (int)(mpu6050.getGyroZoffset() * 100.0) + 0.5);

    Serial.println(eeprom_word_read(0), HEX);
    Serial.println(((float)eeprom_word_read(1)) / 100.0, DEC);
    Serial.println(((float)eeprom_word_read(2)) / 100.0, DEC);
    Serial.println(((float)eeprom_word_read(3)) / 100.0, DEC);
  }
  pinMode(greenLED, OUTPUT);
  pinMode(blueLED, OUTPUT);
}

//=============================================================================

void loop() {

//....
// === Read acceleromter data === //
  Wire.beginTransmission(MPU);
  Wire.write(0x3B); // Start with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true); // Read 6 registers total, each axis value is stored in 2 registers
  //For a range of +-2g, we need to divide the raw values by 16384, according to the datasheet
  AccX = (Wire.read() << 8 | Wire.read()) / 16384.0; // X-axis value
  AccY = (Wire.read() << 8 | Wire.read()) / 16384.0; // Y-axis value
  AccZ = (Wire.read() << 8 | Wire.read()) / 16384.0; // Z-axis value
  // Calculating Roll and Pitch from the accelerometer data
  accAngleX = (atan(AccY / sqrt(pow(AccX, 2) + pow(AccZ, 2))) * 180 / PI) - 0.58; // AccErrorX ~(0.58) See the calculate_IMU_error()custom function for more details
  accAngleY = (atan(-1 * AccX / sqrt(pow(AccY, 2) + pow(AccZ, 2))) * 180 / PI) + 1.58; // AccErrorY ~(-1.58)
  // === Read gyroscope data === //
  previousTime = currentTime;        // Previous time is stored before the actual time read
  currentTime = millis();            // Current time actual time read
  elapsedTime = (currentTime - previousTime) / 500; // Divide by 1000 to get seconds - si on divise par moins, augmente la sensibilité
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
  // Currently the raw values are in degrees per seconds, deg/s, so we need to multiply by sendonds (s) to get the angle in degrees
  gyroAngleX = gyroAngleX + GyroX * elapsedTime; // deg/s * s = deg
  gyroAngleY = gyroAngleY + GyroY * elapsedTime;
  yaw =  yaw + GyroZ * elapsedTime;
  // Complementary filter - combine acceleromter and gyro angle values
  roll = 0.96 * gyroAngleX + 0.04 * accAngleX;
  pitch = 0.96 * gyroAngleY + 0.04 * accAngleY;
  
  // Print the values on the serial monitor
  Serial.print(roll);
  Serial.print(" / ");
  Serial.print(pitch);
  Serial.print(" / ");
  Serial.println(yaw);
  
//...

  if ((Serial.available() > 0)|| first_time == true) {
    blink(50);
    char result = Serial.read();
    //       Serial.println(result);

//-----------------------------------------------------------------------------

    if (result == 'R') {
      Serial.println("OK");
      delay(100);
      first_time = true;
      setup();
    }

//-----------------------------------------------------------------------------

    else if (result == 'C') {                                       //Reboot the data in the EEPROM 
      Serial.println("Clearing stored gyro offsets in EEPROM\n");
      eeprom_word_write(0, 0x00);
      first_time = true;
      setup();
    }

//-----------------------------------------------------------------------------

    if ((result == '?') || first_time == true)
    {
      first_time = false;
      if (bmePresent) {
        Serial.println("OK BME280 : ");
        Serial.print("   Temp = ");
        Serial.println(bme.readTemperature());
        Serial.print("   Pres = ");
        Serial.println(bme.readPressure() / 100.0F);
        Serial.print("   Alti = ");
        Serial.println(bme.readAltitude(SEALEVELPRESSURE_HPA));
        Serial.print("   Humi = ");
        Serial.print(bme.readHumidity());
        Serial.println(" ");
      } else
      {
        Serial.print("OK BME280 0.0 0.0 0.0 0.0");
      }
      mpu6050.update();

      Serial.println("   MPU6050 : ");
      Serial.print("   GX = ");
      Serial.print(mpu6050.getGyroX());
      Serial.print(" ; GY = ");
      Serial.print(mpu6050.getGyroY());
      Serial.print(" ; GZ = ");
      Serial.println(mpu6050.getGyroZ());

      Serial.print("   AX = ");

    //Serial.print(" ");
    Serial.print(mpu6050.getAccX());   
    Serial.print(" ; AY = ");
    Serial.print(mpu6050.getAccY());   
    Serial.print(" ; AZ = ");
    Serial.print(mpu6050.getAccZ());  

    sensorValue = analogRead(A3);
    Serial.println(sensorValue);  
    Temp = T1 + (sensorValue - R1) *((T2 - T1)/(R2 - R1));

    
    //Serial.print(" XS ");
    //Serial.print(Temp);   
    //Serial.print(" ");
    //Serial.println(Sensor1);               
      
    float rotation = sqrt(mpu6050.getGyroX()*mpu6050.getGyroX() + mpu6050.getGyroY()*mpu6050.getGyroY() + mpu6050.getGyroZ()*mpu6050.getGyroZ()); 
    float acceleration = sqrt(mpu6050.getAccX()*mpu6050.getAccX() + mpu6050.getAccY()*mpu6050.getAccY() + mpu6050.getAccZ()*mpu6050.getAccZ()); 
    Serial.print("   Rota = ");
    Serial.println(rotation);
    Serial.print("   Acce = ");
    Serial.println(acceleration);

    if (acceleration > 1.2)
        led_set(greenLED, HIGH);      //the green led turn on with the value of acceleration
    else
        led_set(greenLED, LOW);
        
    if (rotation > 5)
        led_set(blueLED, HIGH);       //the blue led turn on with the value of acceleration
    else
        led_set(blueLED, LOW);          
    }    
  }

delay(10000);

//-----------------------------------------------------------------------------

  if (Serial1.available() > 0) {
 
    blink(50);
    char result = Serial1.read();
    //    Serial1.println(result);

//-----------------------------------------------------------------------------

    if (result == 'R') {              //If we use R in the serial monitor, we have the same result that the init
      Serial1.println("OK");
      delay(100);
      first_read = true;
      setup();
    }

//-----------------------------------------------------------------------------

    if (result == '?')
    {
      if (bmePresent) {
        Serial1.print("OK BME280 ");
        Serial1.print(bme.readTemperature());
        Serial1.print(" ");
        Serial1.print(bme.readPressure() / 100.0F);
        Serial1.print(" ");
        Serial1.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
        Serial1.print(" ");
        Serial1.print(bme.readHumidity());
      } else
      {
        Serial1.print("OK BME280 0.0 0.0 0.0 0.0");
      }
      mpu6050.update();

      Serial1.print(" MPU6050 ");
      Serial1.print(mpu6050.getGyroX());
      Serial1.print(" ");
      Serial1.print(mpu6050.getGyroY());
      Serial1.print(" ");
      Serial1.print(mpu6050.getGyroZ());

    Serial1.print(" ");
    Serial1.print(mpu6050.getAccX());   
    Serial1.print(" ");
    Serial1.print(mpu6050.getAccY());   
    Serial1.print(" ");
    Serial1.print(mpu6050.getAccZ());   

    sensorValue = analogRead(A3);
    //Serial.println(sensorValue);  
    Temp = T1 + (sensorValue - R1) *((T2 - T1)/(R2 - R1));

    Serial1.print(" XS ");
    Serial1.print(Temp);   
    Serial1.print(" ");
    Serial1.println(Sensor2);              
     
    float rotation = sqrt(mpu6050.getGyroX()*mpu6050.getGyroX() + mpu6050.getGyroY()*mpu6050.getGyroY() + mpu6050.getGyroZ()*mpu6050.getGyroZ()); 
    float acceleration = sqrt(mpu6050.getAccX()*mpu6050.getAccX() + mpu6050.getAccY()*mpu6050.getAccY() + mpu6050.getAccZ()*mpu6050.getAccZ()); 
//    Serial.print(rotation);
//    Serial.print(" ");
//    Serial.println(acceleration);


//-----------------------------------------------------------------------------

    if (first_read == true) {
      first_read = false;
      rest = acceleration;
    }

    if (acceleration > 1.2 * rest)
        led_set(greenLED, HIGH);
    else
        led_set(greenLED, LOW);
        
    if (rotation > 5)
        led_set(blueLED, HIGH);
    else
        led_set(blueLED, LOW);
    }
  }

//  delay(100);
}

//-----------------------------------------------------------------------------

void eeprom_word_write(int addr, int val)
{
  EEPROM.write(addr * 2, lowByte(val));
  EEPROM.write(addr * 2 + 1, highByte(val));
}

//-----------------------------------------------------------------------------

short eeprom_word_read(int addr)
{
  return ((EEPROM.read(addr * 2 + 1) << 8) | EEPROM.read(addr * 2));
}

//-----------------------------------------------------------------------------

void blink_setup() 
{
#if defined(ARDUINO_ARCH_STM32F0) || defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F3) || defined(ARDUINO_ARCH_STM32F4) || defined(ARDUINO_ARCH_STM32L4)  
  // initialize digital pin PB1 as an output.
  pinMode(PC13, OUTPUT);
  pinMode(PB9, OUTPUT);
  pinMode(PB8, OUTPUT);
#endif

//-----------------------------------------------------------------------------

#if defined __AVR_ATmega32U4__
  pinMode(RXLED, OUTPUT);  // Set RX LED as an output
  // TX LED is set as an output behind the scenes
  pinMode(greenLED, OUTPUT);
  pinMode(blueLED,OUTPUT);
#endif
}

//-----------------------------------------------------------------------------

void blink(int length)
{
#if defined(ARDUINO_ARCH_STM32F0) || defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F3) || defined(ARDUINO_ARCH_STM32F4) || defined(ARDUINO_ARCH_STM32L4)
  digitalWrite(PC13, LOW);   // turn the LED on (HIGH is the voltage level)
#endif

//-----------------------------------------------------------------------------

#if defined __AVR_ATmega32U4__
  digitalWrite(RXLED, LOW);   // set the RX LED ON
  TXLED0; //TX LED is not tied to a normally controlled pin so a macro is needed, turn LED OFF
#endif  

//-----------------------------------------------------------------------------

  delay(length);              // wait for a lenth of time

//-----------------------------------------------------------------------------

#if defined(ARDUINO_ARCH_STM32F0) || defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F3) || defined(ARDUINO_ARCH_STM32F4) || defined(ARDUINO_ARCH_STM32L4)
  digitalWrite(PC13, HIGH);    // turn the LED off by making the voltage LOW
#endif

//-----------------------------------------------------------------------------

#if defined __AVR_ATmega32U4__
  digitalWrite(RXLED, HIGH);    // set the RX LED OFF
  TXLED0; //TX LED macro to turn LED ON
#endif  
}

//-----------------------------------------------------------------------------

//....
void led_set(int ledPin, bool state)
{
#if defined(ARDUINO_ARCH_STM32F0) || defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F3) || defined(ARDUINO_ARCH_STM32F4) || defined(ARDUINO_ARCH_STM32L4)
  if (ledPin == greenLED)
    digitalWrite(PB9, state);
  else if (ledPin == blueLED)
    digitalWrite(PB8, state);    
#endif

#if defined __AVR_ATmega32U4__
  digitalWrite(ledPin, state);   
#endif  
}

void calculate_IMU_error() {
  // We can call this funtion in the setup section to calculate the accelerometer and gyro data error. From here we will get the error values used in the above equations printed on the Serial Monitor.
  // Note that we should place the IMU flat in order to get the proper values, so that we then can the correct values
  // Read accelerometer values 200 times
  while (c < 200) {
    Wire.beginTransmission(MPU);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU, 6, true);
    AccX = (Wire.read() << 8 | Wire.read()) / 16384.0 ;
    AccY = (Wire.read() << 8 | Wire.read()) / 16384.0 ;
    AccZ = (Wire.read() << 8 | Wire.read()) / 16384.0 ;
    // Sum all readings
    AccErrorX = AccErrorX + ((atan((AccY) / sqrt(pow((AccX), 2) + pow((AccZ), 2))) * 180 / PI));
    AccErrorY = AccErrorY + ((atan(-1 * (AccX) / sqrt(pow((AccY), 2) + pow((AccZ), 2))) * 180 / PI));
    c++;
  }
  //Divide the sum by 200 to get the error value
  AccErrorX = AccErrorX / 200;
  AccErrorY = AccErrorY / 200;
  c = 0;
  // Read gyro values 200 times
  while (c < 200) {
    Wire.beginTransmission(MPU);
    Wire.write(0x43);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU, 6, true);
    GyroX = Wire.read() << 8 | Wire.read();
    GyroY = Wire.read() << 8 | Wire.read();
    GyroZ = Wire.read() << 8 | Wire.read();
    // Sum all readings
    GyroErrorX = GyroErrorX + (GyroX / 131.0);
    GyroErrorY = GyroErrorY + (GyroY / 131.0);
    GyroErrorZ = GyroErrorZ + (GyroZ / 131.0);
    c++;
  }
  //Divide the sum by 200 to get the error value
  GyroErrorX = GyroErrorX / 200;
  GyroErrorY = GyroErrorY / 200;
  GyroErrorZ = GyroErrorZ / 200;
  // Print the error values on the Serial Monitor
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
//....
