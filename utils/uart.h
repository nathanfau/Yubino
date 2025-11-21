#ifndef UART_H
#define UART_H

#include <stdint.h>

void UART__init(void);
uint16_t UART__getc(void);
void UART__putc(uint16_t data);

#endif
