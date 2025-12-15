#include "pressure-sensor-bmp280.h"

#include <iostream>
#include <fcntl.h>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string>
#include <cstring>

#ifdef __linux__
#include <linux/i2c-dev.h>
#else
// Mock I2C definitions for non-Linux platforms (e.g., macOS)
#define I2C_SLAVE 0x0703
#endif

using namespace std;

#define BMP280_REG_CONTROL 0xF4
#define BMP280_REG_CONFIG  0xF5
#define BMP280_REG_PRESSURE_MSB 0xF7
#define BMP280_REG_CALIB 0x88

BMP280::BMP280(const std::string& i2c_device, unsigned address)
    : _i2c(i2c_device, address)
{
    // Konfiguration schreiben
    uint8_t config1[2] = {BMP280_REG_CONTROL, 0x27};  // Temp+Press oversampling x1, normal mode
    uint8_t config2[2] = {BMP280_REG_CONFIG, 0x00};   // Standby 0.5ms, filter off

    if (_i2c.write(config1, 2) != 2) {
        throw std::runtime_error("Failed to configure BMP280.");
    }

    if (_i2c.write(config2, 2) != 2) {
        throw std::runtime_error("Failed to configure BMP280.");
    }

    // Wartezeit, um den Sensor zu stabilisieren
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Kalibrierdaten lesen
    uint8_t calib_reg = BMP280_REG_CALIB;
    if (_i2c.write(&calib_reg, 1) != 1) {
        throw std::runtime_error("Failed to request calibration data.");
    }

    uint8_t calib_data[24];
    if (_i2c.read(calib_data, 24) != 24) {
        throw std::runtime_error("Failed to read calibration data.");
    }

    // Kalibrierdaten entpacken
    _dig_T1 = (calib_data[1] << 8) | calib_data[0];
    _dig_T2 = (calib_data[3] << 8) | calib_data[2];
    _dig_T3 = (calib_data[5] << 8) | calib_data[4];
    _dig_P1 = (calib_data[7] << 8) | calib_data[6];
    _dig_P2 = (calib_data[9] << 8) | calib_data[8];
    _dig_P3 = (calib_data[11] << 8) | calib_data[10];
    _dig_P4 = (calib_data[13] << 8) | calib_data[12];
    _dig_P5 = (calib_data[15] << 8) | calib_data[14];
    _dig_P6 = (calib_data[17] << 8) | calib_data[16];
    _dig_P7 = (calib_data[19] << 8) | calib_data[18];
    _dig_P8 = (calib_data[21] << 8) | calib_data[20];
    _dig_P9 = (calib_data[23] << 8) | calib_data[22];
}

float BMP280::get_value() const
{
    // Druckdaten lesen
    uint8_t press_reg = BMP280_REG_PRESSURE_MSB;
    if (_i2c.write(&press_reg, 1) != 1) {
        throw std::runtime_error("Failed to request pressure data.");
    }

    uint8_t raw_data[6];
    if (_i2c.read(raw_data, 6) != 6) {
        throw std::runtime_error("Failed to read pressure data.");
    }

    int32_t raw_pressure = (raw_data[0] << 12) | (raw_data[1] << 4) | (raw_data[2] >> 4);
    int32_t raw_temp = (raw_data[3] << 12) | (raw_data[4] << 4) | (raw_data[5] >> 4);

    // Temperaturkompensation nach Datenblatt
    int32_t t_fine;
    int32_t var1, var2;

    var1 = (((raw_temp >> 3) - ((int32_t)_dig_T1 << 1)) * ((int32_t)_dig_T2)) >> 11;
    var2 = (((((raw_temp >> 4) - ((int32_t)_dig_T1)) * ((raw_temp >> 4) - ((int32_t)_dig_T1))) >> 12) * ((int32_t)_dig_T3)) >> 14;
    t_fine = var1 + var2;

    // Druckkompensation nach Datenblatt
    int64_t p_var1, p_var2, p;
    p_var1 = ((int64_t)t_fine) - 128000;
    p_var2 = p_var1 * p_var1 * (int64_t)_dig_P6;
    p_var2 = p_var2 + ((p_var1 * (int64_t)_dig_P5) << 17);
    p_var2 = p_var2 + (((int64_t)_dig_P4) << 35);
    p_var1 = ((p_var1 * p_var1 * (int64_t)_dig_P3) >> 8) + ((p_var1 * (int64_t)_dig_P2) << 12);
    p_var1 = (((((int64_t)1) << 47) + p_var1)) * ((int64_t)_dig_P1) >> 33;

    double pressure = 0;
    if (p_var1 != 0) {
        p = 1048576 - raw_pressure;
        p = (((p << 31) - p_var2) * 3125) / p_var1;
        p_var1 = (((int64_t)_dig_P9) * (p >> 13) * (p >> 13)) >> 25;
        p_var2 = (((int64_t)_dig_P8) * p) >> 19;
        p = ((p + p_var1 + p_var2) >> 8) + (((int64_t)_dig_P7) << 4);
        pressure = (double)p / 256.0 / 100.0;
    }

    return pressure;
}

void BMP280::set_value(float value)
{
    throw std::runtime_error("BMP280: set_value() is not supported for real sensors.");
}

BMP280::~BMP280() {}