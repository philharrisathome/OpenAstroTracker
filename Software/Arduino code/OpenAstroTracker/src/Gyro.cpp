#include "../Configuration_adv.hpp"

#if USE_GYRO_LEVEL == 1
#include <Wire.h> // I2C communication library

#include "Utility.hpp"
#include "Gyro.hpp"

const int MPU = 0x68; // I2C address of the MPU6050 accelerometer

bool Gyro::isPresent(false);

void Gyro::startup()
{
    // Initialize interface to the MPU6050
    LOGV1(DEBUG_INFO, F("GYRO:: Starting"));
    Wire.begin();

    Wire.beginTransmission(MPU);
    Wire.write(0x75);   // WHO_AM_I
    Wire.endTransmission(true);
    Wire.requestFrom(MPU, 1, 1);
    byte id = (Wire.read() >> 1) & 0x3F;    
    isPresent = (id == 0x34);
    if (!isPresent) {
        LOGV1(DEBUG_INFO, F("GYRO:: Not found!"));
        return;
    }

    Wire.beginTransmission(MPU);
    Wire.write(0x6B);   // PWR_MGMT_1
    Wire.write(0);      // Disable sleep, 8 MHz clock
    Wire.endTransmission(true);
    Wire.beginTransmission(MPU);
    Wire.write(0x1A);   // CONFIG
    Wire.write(6);      // 5Hz bandwidth
    Wire.endTransmission(true);

    LOGV1(DEBUG_INFO, F("GYRO:: Started"));
}

void Gyro::shutdown()
{
    LOGV1(DEBUG_INFO, F("GYRO: Shutdown"));
    // Nothing to do
}

angle_t Gyro::getCurrentAngles()
{
    const int windowSize = 16;
    // Read the accelerometer data
    struct angle_t result;
    result.pitchAngle = 0;
    result.rollAngle = 0;
    if (!isPresent)
        return result;     // Gyro is not available

    for (int i = 0; i < windowSize; i++)
    {
        Wire.beginTransmission(MPU);
        Wire.write(0x3B); // Start with register 0x3B (ACCEL_XOUT_H)
        Wire.endTransmission(false);
        Wire.requestFrom(MPU, 6, 1);       // Read 6 registers total, each axis value is stored in 2 registers
        int16_t AcX = Wire.read() << 8 | Wire.read(); // X-axis value
        int16_t AcY = Wire.read() << 8 | Wire.read(); // Y-axis value
        int16_t AcZ = Wire.read() << 8 | Wire.read(); // Z-axis value

        // Calculating the Pitch angle (rotation around Y-axis)
        result.pitchAngle += ((atan(-1 * AcX / sqrt(pow(AcY, 2) + pow(AcZ, 2))) * 180.0 / PI) * 2.0) / 2.0;
        // Calculating the Roll angle (rotation around X-axis)
        result.rollAngle += ((atan(-1 * AcY / sqrt(pow(AcX, 2) + pow(AcZ, 2))) * 180.0 / PI) * 2.0) / 2.0;

        delay(10);  // Decorrelate measurements
    }

    result.pitchAngle /= windowSize;
    result.rollAngle /= windowSize;
#if GYRO_AXIS_SWAP == 1
    float temp = result.pitchAngle;
    result.pitchAngle = result.rollAngle;
    result.rollAngle = temp;
#endif
    return result;
}

float Gyro::getCurrentTemperature()
{
    if (!isPresent)
        return 99.9;     // Gyro is not available

    // Read the temperature data
    float result = 0.0;
    int16_t tempValue;
    Wire.beginTransmission(MPU);
    Wire.write(0x41); // Start with register 0x41 (TEMP_OUT_H)
    Wire.endTransmission(false);
    Wire.requestFrom(MPU, 2, 1);             // Read 2 registers total, the temperature value is stored in 2 registers
    tempValue = Wire.read() << 8 | Wire.read(); // Raw Temperature value
    // Calculating the actual temperature value
    result = float(tempValue) / 340 + 36.53;
    return result;
}
#endif
