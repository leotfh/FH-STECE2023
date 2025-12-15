#include "adc-ads1115.h"

#include <stdexcept>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

// ADS1115 Register-Konstanten
constexpr uint8_t ADS1115_REG_CONVERSION = 0x00;
constexpr uint8_t ADS1115_REG_CONFIG = 0x01;

Ads1115::Ads1115(const std::string& i2c_device, unsigned address)
    : _i2c(i2c_device, address), voltage_multiplier(4.096f / 32767.0f)
    {
        // Konfiguriere den ADC im Continuous-Conversion-Modus
        uint8_t config_buffer[3] = {
            ADS1115_REG_CONFIG,
            0xC2, // Config MSB: Continuous conversion, AIN0, Gain +/-4.096V
            0x83  // Config LSB: 128 SPS, Standard-Komparator
        };
    
        if (_i2c.write(config_buffer, 3) != 3) {
            throw std::runtime_error("Konfiguration (continuous mode) konnte nicht auf den Sensor geschrieben werden.");
        }
    
        // Kurze Wartezeit, bis der erste Wert bereit ist (ca. 8 ms)
        usleep(8 * 1000);
    }

float Ads1115::get_value() const
{
    // Pointer auf das Konvertierungsregister setzen
    uint8_t reg_ptr_buffer[1] = {ADS1115_REG_CONVERSION};
    if (_i2c.write(reg_ptr_buffer, 1) != 1) {
        throw std::runtime_error("Register-Pointer für das Auslesen konnte nicht gesetzt werden.");
    }

    // 16-Bit Messwert lesen
    uint8_t data_buffer[2];
    if (_i2c.read(data_buffer, 2) != 2) {
        throw std::runtime_error("Messdaten konnten nicht vom Sensor gelesen werden.");
    }

    // Bytes zu einem Wert zusammensetzen und in Spannung umrechnen
    int16_t raw_adc = (data_buffer[0] << 8) | data_buffer[1];
    return raw_adc * voltage_multiplier;
}

void Ads1115::set_value(float value)
{
    // Leere Implementierung oder Ausnahme werfen, da ein echter Sensor den Wert nicht manuell setzen kann
    throw std::logic_error("Ads1115: set_value() ist nicht erlaubt, da es sich um einen echten Sensor handelt.");
}