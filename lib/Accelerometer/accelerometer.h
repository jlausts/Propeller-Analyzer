#include <Wire.h>
#include <Arduino.h>

#ifndef ACCELER
#define ACCELER

#define MPU6050_ADDR 0x68
#define PWR_MGMT_1   0x6B
#define ACCEL_XOUT_H 0x3B
#define ACCEL_YOUT_H 0x3D
#define ACCEL_ZOUT_H 0x3F

class Accelerometer {
public:
    bool begin(TwoWire &wirePort = Wire, const uint8_t address = MPU6050_ADDR);
    bool read(float &ax_g, float &ay_g, float &az_g);
    bool read_avg(float &ax_g, float &ay_g, float &az_g, const uint16_t samples = 10);

    // --- Single-axis reads ---
    float readX();
    float readY();
    float readZ();
    void  readX(const int num_readings, uint16_t *const arr);
    void  readY(const int num_readings, uint16_t *const arr);
    void  readZ(const int num_readings, uint16_t *const arr);

private:
    TwoWire *_wire;
    uint8_t _addr;

    uint16_t readAxis(const uint8_t regHigh);
    uint16_t readAxis(const uint8_t regHigh, bool fast);
    void readAxis(const uint8_t regHigh, const int num_readings, uint16_t *const arr);
};


#endif
