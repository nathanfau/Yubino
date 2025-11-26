#include <avr/io.h>
#include <avr/eeprom.h>
#include <stdint.h>

#include "utils/uart.h"
#include "utils/ringbuffer.h"
#include "utils/random.h"
#include "utils/constants.h"

#include "micro-ecc/uECC.h"

#define ENTRY_SIZE 57
#define EEPROM_MAX_SIZE 1024
#define MAX_ENTRIES ((EEPROM_MAX_SIZE - 1) / ENTRY_SIZE)

uint8_t current_private_key[21]; 
uint8_t current_public_key[40];
uint8_t current_credential_id[16];



uint8_t store_credential_in_eeprom(const uint8_t app_id_hash[20], const uint8_t credential_id[16], 
                                                                            const uint8_t private_key[21]) {

    // On consulte le nombre de clés actuellement stockées, cette valeur est conservée l'offset 0                                                                            
    uint8_t nb_entries = eeprom_read_byte((uint8_t*)0);

    if (nb_entries > MAX_ENTRIES)
        return STATUS_ERR_STORAGE_FULL;

    uint16_t index = 0;

    // Recherche d'une entrée existante correspondant à app_id_hash
    for (uint8_t i = 0; i < nb_entries; i++) {
        uint16_t base = 1 + i * ENTRY_SIZE;
        uint8_t match = 1;

        for (uint8_t j = 0; j < 20; j++) {
            if (eeprom_read_byte((uint8_t*)(base + j)) != app_id_hash[j]) {
                match = 0;
                break;
            }
        }

        if (match) {
            index = i;
            goto write_entry;
        }
    }

    // Sinon, nouvelle entrée
    if (nb_entries == MAX_ENTRIES)
        return STATUS_ERR_STORAGE_FULL;

    index = nb_entries;

write_entry:;
    uint16_t base = 1 + index * ENTRY_SIZE;

    // 1) SHA1(app_id)
    for (uint8_t i = 0; i < 20; i++)
        eeprom_write_byte((uint8_t*)(base + i), app_id_hash[i]);

    // 2) credential_id
    for (uint8_t i = 0; i < 16; i++)
        eeprom_write_byte((uint8_t*)(base + 20 + i), credential_id[i]);

    // 3) private_key
    for (uint8_t i = 0; i < 21; i++)
        eeprom_write_byte((uint8_t*)(base + 36 + i), private_key[i]);

    // Mettre à jour nb_entries uniquement pour un ajout
    if (index == nb_entries)
        eeprom_write_byte((uint8_t*)0, nb_entries + 1);

    return STATUS_OK;
}



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