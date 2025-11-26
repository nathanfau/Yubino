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

uint8_t store_credential_in_eeprom(const uint8_t app_id_hash[20], const uint8_t credential_id[16], const uint8_t private_key[21]) {

    // On consulte le nombre de clés actuellement stockées, cette valeur est conservée l'offset 0                                                                            
    uint8_t nb_entries = eeprom_read_byte((uint8_t*)0);

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

            if (nb_entries >= MAX_ENTRIES)
                return STATUS_ERR_STORAGE_FULL;
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
    int16_t input;
    uint8_t current_app_id_hash[20];
    uint8_t current_private_key[21]; 
    uint8_t current_public_key[40];
    uint8_t current_credential_id[16];

    for (int i = 0; i < 20; i++) {
        while ((input = UART__getc()) == -1){}
        
        current_app_id_hash[i] = (uint8_t)input;
    }
    // ON DOIT VERIFIER LE SHA 1 ICI
    // si erreur , renvoyer : STATUS_ERR_BAD_PARAMETER

    // DEMANDER LE CONSENTEMENT ICI
    // si on ne donne pas son accord en 10sec, renvoyer: STATUS_ERR_APROVAL
    //(implémenter une version sans le consentement pour les tests)

    // On utilise la bibliothèque importée pour générer un couple clef privée/publique
    int success = uECC_make_key(current_public_key, current_private_key, uECC_secp160r1());

    if (!success) {
        UART__putc(STATUS_ERR_CRYPTO_FAILED);
        return;
    }

    // on appelle notre fonction de random pour generer 16 octets : c'est credential_id
    random__get(current_credential_id, 16);

    // IL FAUT SAUVEGARDER DANS L'EEPROM A CE MOMENT LA 
    // current_app_id_hash || current_credential_id || curren_private_key = 57 octets
    // VERIFIER SI ON A LA PLACE POUR LA SAUVEGARDE sinon, renvoyer: STATUS_ERR_STORAGE_FULL
    // Si un entree avec ap_id existe deja: on ecrase

    // on revoie notre reponse
    // STATUS_OK || credential_id || public_key

    UART__putc(STATUS_OK);
    for (int i = 0; i < 16; i++) {
        UART__putc(current_credential_id[i]);
    }
    for (int i = 0; i < 40; i++) {
        UART__putc(current_public_key[i]);
    }
}

void handle_get_assertion(void) {
    uint8_t current_client_data_hash[20];
    uint8_t current_app_id_hash[20];
    uint8_t current_private_key[21];
    uint8_t signature[40];
    uint8_t status;
    uint16_t input;

    for (int i = 0; i < 20; i++) {
        while ((input = UART__getc()) == -1){}
        
        current_app_id_hash[i] = (uint8_t)input;
    }

    // MEME SOUCIS QUE DANS LA FONCTION PRECEDENTE: STATUS_ERR_BAD_PARAMETER

    // on lit le clientDataHash
    for (int i = 0; i < 20; i++) {
        while ( (input = UART__getc()) == -1); 
        current_client_data_hash[i] = (uint8_t)input;
    }

    // MEME SOUCIS QU'AU DESSUS : STATUS_ERR_BAD_PARAMETER

    // ON CHERCHER DANS NOTRE MEMOIRE LES INFOS : credential_id et private_key
    // ces infos sont associés au app_id_hash
    // SI ON NE TROUVE PAS D'ENTREE ASSOCIE ON RENVOIE: STATUS_ERR_NOT_FOUND

    // IL FAUT STOCKER DANS UN VARIABLE DE LA FONCTION LA VALEUR DE crediential_id


    // CONSENTEMENT sinon STATUS_ERR_APPROVAL



    // SIGNATURE 
    int crypto_success = uECC_sign(current_private_key, current_client_data_hash, 20, signature, uECC_secp160r1());

    if (!crypto_success) {
        UART__putc(STATUS_ERR_CRYPTO_FAILED);
        return;
    }

    // on revoie notre reponse
    // STATUS_OK || credential_id || signature
    UART__putc(STATUS_OK);
    //for (int i = 0; i < 16; i++) {
        //UART__putc(current_credential_id[i]);
    //}
    for (int i = 0; i < 40; i++) {
        UART__putc(signature[i]);
    }
}









int main(void) {
    UART__init();

    // STORAGE__INIT()

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

                case COMMAND_GET_ASSERTION:
                    handle_get_assertion();
                    break;
                
                default:
                    UART__putc(STATUS_ERR_COMMAND_UNKNOWN);
                    break;
            }
        }
    }
    return 0;
}