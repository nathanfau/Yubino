#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include "consent.h"

volatile uint8_t button_state = 1;
volatile uint8_t debounce_counter = 0;   
volatile uint8_t button_pushed_flag = 0;

volatile uint8_t demi_seconds_counter = 0;
volatile uint8_t timeout_flag = 0;


// algorithme proposé dans le sujet du TP5
void debounce(void){
    uint8_t current_button_state = (PIND & (1 << PD2));
 
    if (current_button_state != button_state) {

        debounce_counter++;
        if (debounce_counter >= 4) {
            button_state = current_button_state; 
            debounce_counter = 0;
            
            if (button_state == 0) {
                button_pushed_flag = 1;
            }
        }
    } else {
        debounce_counter = 0;
    }
}

//gestion de l'interruption du watchdog timer
ISR(WDT_vect){
    debounce();

    // Remet à zéro le timer
    wdt_reset();

    // Interruption enable
    WDTCSR |= (1 << WDIE);
}

// gestion de l'interruption du timer1 (toutes les 500ms)
ISR(TIMER1_COMPA_vect) {
    demi_seconds_counter++;

    // On inverse l'etat de la led
    PORTB ^= (1 << PB5);

    // 20 * 500ms = 10s
    if (demi_seconds_counter >= 20) {
        //lorsqu'on a atteint 10s, on leve un flag
        timeout_flag = 1;
    }
}

// fonction mere, qui initialise le timer1 et le WDT
uint8_t wait__for__user__consent(void) {

    DDRB |= (1 << PB5);  // PB5 en sortie (LED)
    PORTB |= (1 << PB5);

    // si on a activé le mode BYPASS (flag à la compilation), on return 1 (le consent est automatique)
    #if BYPASS_USER_CONSENT
        return 1;
    #endif

    // PD2 en entree
    PORTD |= (1 << PD2);

    button_pushed_flag = 0;
    button_state = 1; 
    debounce_counter = 0;

    demi_seconds_counter = 0;
    timeout_flag = 0;
    
    // desactive les interrutpions
    cli();

    //met le compteur a 0
    wdt_reset();
    
    //deverouille les bits de modif du WDT
    WDTCSR |= (1 << WDCE) | (1 << WDE);

    // on actives les interruptions et on regle le WDT sur 16ms (le + proche de 15ms)
    WDTCSR = (1 << WDIE) | (0 << WDP3) | (0 << WDP2) | (0 << WDP1) | (0 << WDP0);

    // reset le timer
    TCCR1A = 0;

    // reset le compteur a 0
    TCNT1 = 0;

    // mode CTC + prescaler a 256
    TCCR1B = (1 << WGM12) | (1 << CS12);

    // seuil
    OCR1A = 31249;

    //active l'interruption de comparaison (on compare match ...)
    TIMSK1 |= (1 << OCIE1A);
    
    // reactive les interruptions
    sei();

    // tant qu'il n'y a pas eu d'appui et qu'on a pas encore atteint 10s
    while ( (button_pushed_flag == 0) && (timeout_flag == 0) ) {
        
        set_sleep_mode(SLEEP_MODE_IDLE);
        sleep_enable();
        sleep_cpu();
        sleep_disable();
    }

    // je sors de cette boucle seulemnt si l'utilisateur a donné son consentement ou si le temps est écoulé 

    
    // coupe les interruptions
    cli();

    wdt_reset();

    // pour reset le watchdog
    MCUSR &= ~(1 << WDRF);

    // on deverouille le watchdog (pour pouvoir modifier les bits interne)
    WDTCSR |= (1 << WDCE) | (1 << WDE);
    
    // on remet tout le WDT a 0
    WDTCSR = 0x00;

    // reset du timer1
    TCCR1B = 0;
    TIMSK1 = 0;

    // reactive les interruptions
    sei();

    // return le resultat, c'est à dire: est ce que le bouton a été push dans le temps imparti ?
    if (button_pushed_flag) {
        return 1;
    } else {
        return 0;
    }
}
