#pragma once

#include "analog-sensor.h"
#include "utilities/i2c.h" 
#include <string>
#include <cstdint>

class Ads1115 : public AnalogSensor
{
public:
    Ads1115(
        const std::string& i2c_device = "/dev/i2c-1",
        unsigned address = 0x48  // Standard-I2C-Adresse des ADS1115
    );

    ~Ads1115() override = default;

    float get_value() const override;
    void set_value(float value) override;
    

private:
    I2C _i2c;  // I2C-Objekt für die Kommunikation
    float voltage_multiplier;
};
