#include <stdint.h>
#include "port.h"

uint8_t inb(uint16_t port) {
    uint8_t result;
    asm volatile("in %0, %1" : "=a"(result) : "d"(port));
    return result;
}

void outb(uint16_t port, uint8_t data) {
    asm volatile("out %1, %0" : : "a"(data), "d"(port));
}