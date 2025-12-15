#include "i2c.h"
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

I2C::I2C(const std::string& device, unsigned address) : _fd(-1) {
    _fd = open(device.c_str(), O_RDWR);
    if (_fd < 0) {
        throw std::runtime_error("Failed to open I2C device: " + device);
    }
    if (ioctl(_fd, I2C_SLAVE, address) < 0) {
        close(_fd);
        throw std::runtime_error("Failed to set I2C slave address.");
    }
}

I2C::~I2C() {
    if (_fd >= 0) {
        close(_fd);
    }
}

ssize_t I2C::write(const uint8_t* data, size_t length) const {
    return ::write(_fd, data, length);
}

ssize_t I2C::read(uint8_t* buffer, size_t length) const {
    return ::read(_fd, buffer, length);
}
