#include <stdint.h>
#include <string.h>

#include "uart.h"
#include "ringbuffer.h"

void ring_buffer__init(struct ring_buffer* rb, uint16_t* buffer, uint16_t maxlen){
    rb->buffer = buffer;
    rb->maxlen = maxlen;
    rb->head = 0;
    rb->tail = 0;
}

void ring_buffer__push(struct ring_buffer* rb, uint16_t data){
    uint16_t next = rb->tail + 1;
    if (next >= rb->maxlen) {
        next = 0;
    }

    if (next == rb->head) {
        rb->head++;
        if (rb->head >= rb->maxlen) {
            rb->head = 0;
        }
    }

    rb->buffer[rb->tail] = data;
    rb->tail = next;  
}

uint16_t ring_buffer__pop(struct ring_buffer* rb, uint16_t* data){
    if (rb->head == rb->tail) {
        *data = -1; 
        return 1;
    }

    *data = rb->buffer[rb->head];
    rb->head++;

    if (rb->head >= rb->maxlen) {
        rb->head = 0;
    }

    return 0;
}
