#pragma once
#include <stdint.h>
#include <stddef.h>

namespace drivers::uart {

enum class Id : uint8_t {
    Debug = 0,
    Drive = 1,
    Imu   = 2,
    K230  = 3,
};

void init();

size_t write(Id id, const void* data, size_t len);
size_t read(Id id, void* buf, size_t len);
bool   readable(Id id);

}