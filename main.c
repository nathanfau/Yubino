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

    set_sleep_mode(SLEEP_MODE_IDLE);

    while (1) {

        cli();
        
        int16_t input = UART__getc();
        
        if (input != -1) {

            sei();

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
                
                default:
                    UART__putc(STATUS_ERR_COMMAND_UNKNOWN);
                    break;
            }
        } else {
            sleep_enable(); 
            sei(); 
            
            sleep_cpu(); 
            
            sleep_disable(); 
        }
    }
    return 0;
}