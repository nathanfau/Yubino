#include <avr/io.h>
#include "random.h"

// initialise l'ADC et le timer0 avant de commencer à les utiliser pour générer de l'aléa
void random__init(void) {
    // selection de la tension de référence (Vref) : AVcc (5V)
    ADMUX = (1 << REFS0);
    // les bits MUX3 || MUX2 || MUX1 || MUX0 sont à 0 donc on écoute le port ADC0
    // rien n'est branché dessus donc c'est juste du bruit 

    // ADEN pour enable l'ADC
    // ADPS2 et ADPS1 pour set le prescaler à 64 : frequence de 16Mhz / 64 =  250khz, un frequence plus lente serait + précise mais on veut juste du bruit ici
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);

    // On init le timer0 (on remet tous les bits à 0)
    // on veut le mode normal (pas d'interruption ni de compare match etc)
    TCCR0A = 0;

    // Pas de prescaler, tourne a 16MHz (très rapidement, pas la même vitesse que l'ADC)
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

        // au prochain tour (boucle while), on écrira à l'adresse mémoire suivante
        dest++;
    }
    //return 1 si tout a marché
    return 1;
}