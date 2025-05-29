#ifndef __VALERN_PORT_H
#define __VALERN_PORT_H

uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t data);

#endif // __VALERN_PORT_H
