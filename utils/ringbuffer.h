// ringbuffer.h
#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdint.h>

#define BUFFER_SIZE 128

struct ring_buffer {
    uint16_t *buffer;
    volatile uint16_t head;
    volatile uint16_t tail;
    uint16_t maxlen;
};

void ring_buffer__init(struct ring_buffer* rb, uint16_t* buffer, uint16_t maxlen);
void ring_buffer__push(struct ring_buffer* rb, uint16_t data);
uint16_t ring_buffer__pop(struct ring_buffer* rb, uint16_t* data);

#endif
