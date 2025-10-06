#include "accelerometer.h"

bool Accelerometer::begin(TwoWire &wirePort, const uint8_t address)
{
    _wire = &wirePort;
    _addr = address;
    _wire->begin();
    _wire->setClock(400000);

    // Wake up device (clear sleep bit)
    _wire->beginTransmission(_addr);
    _wire->write(PWR_MGMT_1);
    _wire->write(0x00);
    if (_wire->endTransmission() != 0)
        return false;

    delay(50);
    return true;
}

bool Accelerometer::read(float &ax_g, float &ay_g, float &az_g)
{
    int16_t ax, ay, az;

    _wire->beginTransmission(_addr);
    _wire->write(ACCEL_XOUT_H);
    if (_wire->endTransmission(false) != 0)
        return false;
    if (_wire->requestFrom(_addr, (uint8_t)6) != 6)
        return false;

    ax = (_wire->read() << 8) | _wire->read();
    ay = (_wire->read() << 8) | _wire->read();
    az = (_wire->read() << 8) | _wire->read();

    ax_g = ax / 16384.0f;
    ay_g = ay / 16384.0f;
    az_g = az / 16384.0f;
    return true;
}

bool Accelerometer::read_avg(float &ax_g, float &ay_g, float &az_g, const uint16_t samples)
{
    double sumx = 0, sumy = 0, sumz = 0;
    float x, y, z;
    for (uint16_t i = 0; i < samples; i++)
    {
        if (!read(x, y, z))
            return false;
        sumx += x;
        sumy += y;
        sumz += z;
    }
    ax_g = sumx / samples;
    ay_g = sumy / samples;
    az_g = sumz / samples;
    return true;
}

// --- Single-axis reads ---
float Accelerometer::readX() { return readAxis(ACCEL_XOUT_H); }
float Accelerometer::readY() { return readAxis(ACCEL_YOUT_H); }
float Accelerometer::readZ() { return readAxis(ACCEL_ZOUT_H); }
void Accelerometer::readX(const int num_readings, uint16_t *const arr) { readAxis(ACCEL_XOUT_H, num_readings, arr); }
void Accelerometer::readY(const int num_readings, uint16_t *const arr) { readAxis(ACCEL_YOUT_H, num_readings, arr); }
void Accelerometer::readZ(const int num_readings, uint16_t *const arr) { readAxis(ACCEL_ZOUT_H, num_readings, arr); }

uint16_t Accelerometer::readAxis(const uint8_t regHigh)
{
    _wire->beginTransmission(_addr);
    _wire->write(regHigh);
    _wire->endTransmission(false);
    _wire->requestFrom(_addr, (uint8_t)2, (uint8_t)true);
    return (_wire->read() << 8) | _wire->read();
}

uint16_t Accelerometer::readAxis(const uint8_t regHigh, bool fast)
{
    _wire->beginTransmission(_addr);
    _wire->write(regHigh);
    _wire->endTransmission(false);
    _wire->requestFrom(_addr, (uint8_t)2, (uint8_t)true);
    return (_wire->read() << 8) | _wire->read();
}

void Accelerometer::readAxis(const uint8_t regHigh, const int num_readings, uint16_t *const arr)
{
    for (int i = 0; i < num_readings; ++i)
        arr[i] = readAxis(regHigh);
}

