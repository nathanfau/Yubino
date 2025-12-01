#include <avr/io.h>
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

    // Pas de prescaler, tourne a 16MHz (très rapidement)
    TCCR0B = (1 << CS00); 
}

int random__get(uint8_t *dest, unsigned size) {
    // on boucle tant qu'on a pas généré autant d'octet que prévu
    while (size--) {
        
        // c'est un des octets qu'on génère (on génère 1 octet à la fois), on l'initialise à 0
        uint8_t current = 0;

        // boucle sur chaque bit qu'on veut générer
        for (uint8_t i = 0; i < 8; i++) {
            
            // lance la conversion
            // on lit une valeur (voltage entre 0V et 5V) et on le traduit en un nombre sur 10bits (0 à 1023)
            ADCSRA |= (1 << ADSC);

            // attente active que la conversion se finisse (c'est fini quand ADSC repasse à 0)
            while (ADCSRA & (1 << ADSC));

            // on récupère le bit de poids faible de l'ADC
            uint8_t bit_adc = ADC & 1;
            
            // on récupère le bit de poids faible du timer0
            uint8_t bit_timer = TCNT0 & 1;

            // on combine les valeurs avec un XOR
            // on écrit le résultat au bit i de notre variable current
            if (bit_adc ^ bit_timer) {
                current |= (1 << i);
            }
        }
        
        // on écrit le résultat à l'adresse pointée
        *dest = current;

        // au prochain tour (boucle while), on écrira dans à l'adresse mémoire suivante
        dest++;
    }
    //return 1 si tout a marché
    return 1;
}