#include <avr/io.h>
#include <stdint.h>

#include "utils/uart.h"
#include "utils/ringbuffer.h"
#include "utils/random.h"
#include "utils/constants.h"

#include "micro-ecc/uECC.h"

uint8_t current_private_key[21]; 
uint8_t current_public_key[40];
uint8_t current_credential_id[16];

void handle_make_credential(void) {
    // 1. Lire le SHA1(app_id) envoyé par le client (20 octets)
    // Le protocole impose de lire ces données même si on ne les stocke pas encore.
    for (int i = 0; i < 20; i++) {
        while (UART__getc() == -1); // Attente bloquante simple
    }

    // 2. Générer la paire de clés ECC
    // uECC_secp160r1() est disponible car défini dans le Makefile
    int success = uECC_make_key(current_public_key, current_private_key, uECC_secp160r1());

    if (!success) {
        UART__putc(STATUS_ERR_CRYPTO_FAILED);
        return;
    }

    // 3. Générer un Credential ID aléatoire (16 octets)
    random__get(current_credential_id, 16);

    // 4. Envoyer la réponse au client
    UART__putc(STATUS_OK); // Statut

    // Envoi credential_id (16 octets)
    for (int i = 0; i < 16; i++) {
        UART__putc(current_credential_id[i]);
    }

    // Envoi public_key (40 octets)
    for (int i = 0; i < 40; i++) {
        UART__putc(current_public_key[i]);
    }
}

int main(void) {
    UART__init();

    random__init();
    uECC_set_rng(random__get);

    while (1) {
        int16_t input = UART__getc();
        
        if (input != -1) {
            uint8_t cmd = (uint8_t)input;

            switch (cmd) {
                case COMMAND_MAKE_CREDENTIAL:
                    handle_make_credential();
                    break;
                
                default:
                    UART__putc(STATUS_ERR_COMMAND_UNKNOWN);
                    break;
            }
        }
    }
    return 0;
}