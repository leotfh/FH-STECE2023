#pragma once
#include <string>
#include <cstdint>

class I2C {
public:
    I2C(const std::string& device, unsigned address);
    ~I2C();

    ssize_t write(const uint8_t* data, size_t length) const;
    ssize_t read(uint8_t* buffer, size_t length) const;

private:
    int _fd;
};
