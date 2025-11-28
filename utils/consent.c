#include <avr/io.h>
#include <util/delay.h>
#include "consent.h"

uint8_t wait_for_user_consent() {

#if BYPASS_USER_CONSENT
    return 1;
#else
    uint16_t timer = 0;

    while (timer < 10000) {
        if (button_is_pressed()) return 1;
        toggle_led_if_needed();
        _delay_ms(10);
        timer += 10;
    }

    return 0;
#endif
}
