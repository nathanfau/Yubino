#include <avr/io.h>
#include <stdint.h>
#include <stdio.h>
#include <avr/interrupt.h>

#include "uart.h"
#include "ringbuffer.h"

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#define BAUD 115200UL
#define UBRR_VALUE (F_CPU / (8UL * BAUD) - 1UL)

uint16_t buffer[BUFFER_SIZE];                
struct ring_buffer rb;

int uart_putchar(char c, FILE *stream);
int uart_getchar(FILE *stream);

// On redirige les flux standards stdin, stdout et stderr à vers nos fonctions UART.
// ELle prend 3 arguments : une fonction put, une fonction get, et un mode de lecture/écriture
FILE uart_stream = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

void UART__init(void) {
    UBRR0H = (unsigned char)(UBRR_VALUE >> 8);
    UBRR0L = (unsigned char)(UBRR_VALUE);           // on initialise le baud rate

    UCSR0A |= (1 << U2X0); // Double the USART Transmission Speed == car on a divise UBRR par 8 et non pas par 16
    // voir entre autre page 199 de la doc (pdf de 662pages)

    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); //  définit la taille de nos messages à 8bits

    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0); // active la reception, l'emission et l'interruption RX

    ring_buffer__init(&rb, buffer, BUFFER_SIZE); // on init le ring buffer

    // On redirige les flux standards
    stdout = &uart_stream;
    stdin  = &uart_stream;
    stderr = &uart_stream;

    sei(); // activer les interruptions globales
}

uint16_t UART__getc() {
    uint16_t data;
    
    if (ring_buffer__pop(&rb, &data) == 0){ 
        return data;
    }
    return -1;
}

void UART__putc(uint16_t data) {
    while (!(UCSR0A & (1 << UDRE0)));           //tant que le registre n'est pas dispo , attendre
    UDR0 = data;                                // renvoyer dans le terminal le caractere
}

ISR(USART_RX_vect) {
    uint16_t data = UDR0;
    ring_buffer__push(&rb, data);
}

// Fonction compatible avec printf()
int uart_putchar(char c, FILE *stream) {
    if (c == '\n') {
        UART__putc('\r');
    }
    UART__putc(c);
    return 0;
}

// Fonction compatible avec gets()/fgets()
int uart_getchar(FILE *stream) {
    uint16_t data = UART__getc();
    if (data ==-1){
        return _FDEV_EOF;
    }
    return data;
}
