#include <stdint.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

#include "utils/uart.h"
#include "utils/random.h"
#include "utils/constants.h"
#include "utils/consent.h"

#include "micro-ecc/uECC.h"
#include "utils/storage.h"
#include "utils/handlers.h"


int main(void) {
    UART__init();

    random__init();
    uECC_set_rng(random__get);

    // on désactive le timer2
    PRR |= ( 1 << PRTIM2);

    // on justifie le choix de ce mode de sommeil dans le README
    set_sleep_mode(SLEEP_MODE_IDLE);

    sei();

    while (1) {
        
        int16_t input = UART__getc();
        
        // si le caractère reçu correspond à quelque chose
        if (input != -1) {

            // le caractère recu est le code d'une commande (voir fichier constants.h)
            uint8_t cmd = (uint8_t)input;

            switch (cmd) {
                case COMMAND_MAKE_CREDENTIAL:
                    handle__make__credential();
                    break;

                case COMMAND_GET_ASSERTION:
                    handle__get__assertion();
                    break;

                case COMMAND_LIST_CREDENTIALS:
                    handle__list__credentials();
                    break;

                case COMMAND_RESET:
                    handle__reset();
                    break;
                
                // commande non reconnue
                default:
                    UART__putc(STATUS_ERR_COMMAND_UNKNOWN);
                    break;
            }
        } else {
            // on autorise le microcontrolleur à s'endormir
            sleep_enable(); 
            
            // on endort le microcontrolleur
            sleep_cpu(); 
            
            // on desactive le droit à s'endormir de microcontrolleur (sécurité)
            sleep_disable(); 

            // on aurait pu utiliser sleep_mode(...) pour cette section
        }
    }
    return 0;
}