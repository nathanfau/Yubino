#ifndef RANDOM_H
#define RANDOM_H

#include <stdint.h>

void random__init(void);
int random__get(uint8_t *dest, unsigned size);

#endif