#include <avr/io.h>
#include <stdint.h>

#include "utils/uart.h"
#include "utils/ringbuffer.h"
#include "utils/random.h"
#include "utils/constants.h"

#include "micro-ecc/uECC.h"

void handle_make_credential(void) {
    int16_t input;
    uint8_t current_app_id_hash[20];
    uint8_t current_private_key[21]; 
    uint8_t current_public_key[40];
    uint8_t current_credential_id[16];

    uint16_t err_sha1= 1;
    while ((err_sha1 = UART__getc()) == -1){}

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
    
    uint16_t err_sha1= 1;
    while ((err_sha1 = UART__getc()) == -1){}

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