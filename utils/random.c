#include <avr/io.h>
#include <util/delay.h>
#include "random.h"

// initialise l'ADC et le timer0 avant de commencer à les utiliser pour générer de l'aléa
void random__init(void) {
    // A PRECISER //////////////////////////////////////////////////////////////////////////////////////////////////
    // AV_cc with external capacitor at AREF pin
    ADMUX = (1 << REFS0);
    
    // ADEN pour enable l'ADC
    // ADPS2 et ADPS1 pour set le prescaler à 64
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);

    // On init le timer0 (on remet tout les bits à 0)
    // on veut le mode normal (pas d'interruptions ni de compare match etc)
    TCCR0A = 0;

    // Pas de prescaler, tourne a 16MHz
    TCCR0B = (1 << CS00); 
}

// génère 1 octet "aléatoire" en utilisant le bit de poids faible de l'ADC (celui qui "bouge" le plus)
static uint8_t generate_adc_byte(void) {

    // c'est la valeur qu'on renvoit, on l'initialise à 0;
    uint8_t acc = 0;    

    // boucle sur chaque bit qu'on veut générer
    for (uint8_t i = 0; i < 8; i++) {

        // lance la conversion
        // on lit une valeur (voltage entre 0V et 5V) et on le traduit en un nombre sur 10bits (0 à 1023)
        ADCSRA |= (1 << ADSC);

        // attente active que la conversion se finisse (c'est fini quand ADSC repasse à 0)
        while (ADCSRA & (1 << ADSC)){}
        
        // si le dernier bits de l'ADC (càd le bit de poids faible) est à 1 alors on set le bit i de acc à 1 
        // sinon on laisse le bit i de acc à 0
        if (ADC & 1) {
            acc |= (1 << i);
        }
    }

    return acc;
}

// génère 1 octet "aléatoire" en utilisant le timer0
static uint8_t generate_timer_byte(void) {

    // c'est la valeur qu'on renvoit, on l'initialise à 0;
    uint8_t acc = 0;    

    // boucle sur chaque bit qu'on veut générer
    for (uint8_t i = 0; i < 8; i++) {

        // si le dernier bits du timer0 (càd le bit de poids faible) est à 1 alors on set le bit i de acc à 1 
        // sinon on laisse le bit i de acc à 0
        if (TCNT0 & 1) {
            acc |= (1 << i);
        }

        // pour décaler un tout petit peu et que ce soit pas prévisible
        _delay_us(2); 
    }
    return acc;
}

// génère 8 bits "vraiment aléatoire" en combinant 2 octets généres par des sources différentes
int random__get(uint8_t *dest, unsigned size) {

    // on boucle tant qu'on a pas généré autant d'octet que prévu
    while (size--) {

        // on appelle les 2 fonctions précédentes
        uint8_t val_adc = generate_adc_byte();
        uint8_t val_timer = generate_timer_byte();

        // on combine les valeurs avec un XOR
        // on écrit le résultat à l'adresse pointée     
        *dest = val_adc ^ val_timer;
        
        // au prochain tour on écrira dans à l'adresse mémoire suivante
        dest++;
    }
    //return 1 si tout a marché
    return 1;
}