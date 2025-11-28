#include <avr/eeprom.h>
#include <stdint.h>

#include "uart.h"
#include "random.h"
#include "constants.h"
#include "consent.h"
#include "handlers.h"

#include "../micro-ecc/uECC.h"
#include "storage.h"


void handle__make__credential(void) {
    uint8_t app_id_hash[20];
    uint8_t private_key[21]; 
    uint8_t public_key[40];
    uint8_t credential_id[16];

    int16_t input;

    for (int i = 0; i < 20; i++) {
        while ((input = UART__getc()) == -1){}
        
        app_id_hash[i] = (uint8_t)input;
    }
    // ON DOIT VERIFIER LE SHA 1 ICI
    // si erreur , renvoyer : STATUS_ERR_BAD_PARAMETER

    // Placeholder en attendant le bouton :
    uint8_t user_approved = wait_for_user_consent();
    // ---------------------------

    if (!user_approved) {
        UART__putc(STATUS_ERR_APPROVAL);
        return;
    }

    // On utilise la bibliothèque importée pour générer un couple clef privée/publique
    int success_crypto = uECC_make_key(public_key, private_key, uECC_secp160r1());

    if (!success_crypto) {
        UART__putc(STATUS_ERR_CRYPTO_FAILED);
        return;
    }

    // on appelle notre fonction de random pour generer 16 octets : credential_id
    random__get(credential_id, 16);

    // on enregistre le tuple (app_id_hash, credentieal_id, private_key) dans l'eeprom
    uint8_t success_write = store__credential__in__eeprom(app_id_hash, credential_id, private_key);

    // on renvoie le tout au client
    if (success_write != STATUS_OK){
        UART__putc(STATUS_ERR_STORAGE_FULL);
        return;
    }

    UART__putc(STATUS_OK);
    for (int i = 0; i < 16; i++) {
        UART__putc(credential_id[i]);
    }
    for (int i = 0; i < 40; i++) {
        UART__putc(public_key[i]);
    }
}

void handle__get__assertion(void) {
    uint8_t client_data_hash[20];
    uint8_t app_id_hash[20];
    uint8_t private_key[21];
    uint8_t credential_id[16];
    uint8_t signature[40];

    int16_t input;

    for (int i = 0; i < 20; i++) {
        while ((input = UART__getc()) == -1){}
        
        app_id_hash[i] = (uint8_t)input;
    }

    // MEME SOUCIS QUE DANS LA FONCTION PRECEDENTE: STATUS_ERR_BAD_PARAMETER

    // on lit le clientDataHash
    for (int i = 0; i < 20; i++) {
        while ( (input = UART__getc()) == -1); 
        client_data_hash[i] = (uint8_t)input;
    }

    // MEME SOUCIS QU'AU DESSUS : STATUS_ERR_BAD_PARAMETER

    // on cherche une entree avec app_id_hash dans notre eeprom
    uint8_t success_search = storage__search(app_id_hash, credential_id, private_key);

    // on renvoie le tout au client
    if (success_search != STATUS_OK){
        UART__putc(STATUS_ERR_NOT_FOUND);
        return;
    }

    // Placeholder en attendant le bouton :
    uint8_t user_approved = wait_for_user_consent();
    // ---------------------------

    if (!user_approved) {
        UART__putc(STATUS_ERR_APPROVAL);
        return;
    }

    // SIGNATURE 
    int succes_crypto = uECC_sign(private_key, client_data_hash, 20, signature, uECC_secp160r1());

    if (!succes_crypto) {
        UART__putc(STATUS_ERR_CRYPTO_FAILED);
        return;
    }

    // on revoie notre reponse
    // STATUS_OK || credential_id || signature
    UART__putc(STATUS_OK);
    for (int i = 0; i < 16; i++) {
        UART__putc(credential_id[i]);
    }
    for (int i = 0; i < 40; i++) {
        UART__putc(signature[i]);
    }
}

void handle__list__credentials() {
    uint8_t nb_entries = eeprom_read_byte((uint8_t*)0);

    // Si EEPROM vidée (reset), elle contient 0xff
    if (nb_entries == 0xff) {
        nb_entries = 0;
    }

    // On enverra toujours STATUS_OK car la commande ne peut pas échouer
    UART__putc(STATUS_OK);

    // On envoie d'abord le nombre d'entrée
    UART__putc(nb_entries);

    // Pour chaque entrée, on envoie son credential_id et son app_id_hash
    for (uint8_t i = 0; i < nb_entries; i++) {
        uint16_t base = 1 + i * ENTRY_SIZE;

        // credential_id : offset 20 à 35
        for (uint8_t j = 0; j < 16; j++) {
            uint8_t byte = eeprom_read_byte((uint8_t*)(base + 20 + j));
            UART__putc(byte);
        }

        // app_id_hash : offset 0 à 19
        for (uint8_t j = 0; j < 20; j++) {
            uint8_t byte = eeprom_read_byte((uint8_t*)(base + j));
            UART__putc(byte);
        }
    }
}

void handle__reset() {

    // Placeholder en attendant le bouton :
    uint8_t user_approved = wait_for_user_consent();
    // ---------------------------

    if (!user_approved) {
        UART__putc(STATUS_ERR_APPROVAL);
        return;
    }

    uint8_t nb_entries = eeprom_read_byte((uint8_t*)0);

    // Si EEPROM vidée (reset), elle contient 0xff
    if (nb_entries == 0xff) {
        nb_entries = 0;
    }

    // Effacer nb_entries 
    eeprom_update_byte((uint8_t*)0, 0x00);

    uint16_t limit = 1 + nb_entries * ENTRY_SIZE;

    // Effacer le reste (1 à 1023) en fixant à 0x00
    for (uint16_t addr = 0; addr < limit; addr++) {
        eeprom_update_byte((uint8_t*)addr, 0x00);
    }

    UART__putc(STATUS_OK);
}