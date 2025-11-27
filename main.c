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

typedef struct {
    uint8_t nb_entries;
    uint8_t match;
    uint16_t index;
} SearchResult;

SearchResult search__by__app_id_hash (const uint8_t app_id_hash[20]){
    SearchResult out;
    // On consulte le nombre de clés actuellement stockées, cette valeur est conservée l'offset 0   
    out.nb_entries = eeprom_read_byte((uint8_t*)0);

    // après un reset, l'eeprom est rempli de FF
    if (out.nb_entries == 0xFF) {
        out.nb_entries = 0;
    }

    out.index = 0;
    out.match = 0;

    // Recherche d'une entrée existante correspondant à app_id_hash
    for (uint8_t i = 0; i < out.nb_entries; i++) {
        uint16_t base = 1 + i * ENTRY_SIZE;
        uint8_t stored_hash[20];
        
        // On lit app_id_hash pour comparer
        eeprom_read_block(stored_hash, (const void*)base, 20);

        // on compare les 2 
        if (memcmp(stored_hash, app_id_hash, 20) == 0) {
            out.index = i;
            out.match = 1;
            break;
        }
    }

    return out;
}

uint8_t store__credential__in__eeprom(const uint8_t app_id_hash[20], const uint8_t credential_id[16], const uint8_t private_key[21]) {

    // On appelle notre fonction qui cherche une entree par son app_id_hash
    SearchResult in = search__by__app_id_hash(app_id_hash);
    
    if (!in.match) {
        if (in.nb_entries >= MAX_ENTRIES) {
            return STATUS_ERR_STORAGE_FULL;
        }
        in.index = in.nb_entries;
    }

    uint16_t base = 1 + in.index * ENTRY_SIZE;

    // on écrit SHA1(app_id)
    eeprom_update_block(app_id_hash, (void*)(base), 20);

    // on écrit credential_id
    eeprom_update_block(credential_id, (void*)(base + 20), 16);

    // on écrit private_key
    eeprom_update_block(private_key, (void*)(base + 36), 21);

    // Mettre à jour nb_entries uniquement pour un ajout
    if (!in.match){
        eeprom_update_byte((uint8_t*)0, in.nb_entries + 1);
    }

    return STATUS_OK;
}

uint8_t storage__search(const uint8_t *app_id_hash, uint8_t *credential_id, uint8_t *private_key) {

    SearchResult in = search__by__app_id_hash(app_id_hash);

    uint16_t base = 1 + in.index * ENTRY_SIZE;

    if (in.match){
        eeprom_read_block(credential_id, (const void*)(base + 20), 16);
        eeprom_read_block(private_key, (const void*)(base + 36), 21);
        return STATUS_OK;
    }

    return STATUS_ERR_NOT_FOUND;
}

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

    // DEMANDER LE CONSENTEMENT ICI
    // si on ne donne pas son accord en 10sec, renvoyer: STATUS_ERR_APROVAL
    //(implémenter une version sans le consentement pour les tests)

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

    // CONSENTEMENT sinon STATUS_ERR_APPROVAL

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
    uint8_t user_approved = 1;
    // ---------------------------

    if (!user_approved) {
        UART__putc(STATUS_ERR_APPROVAL);
        return;
    }

    // Effacer nb_entries > on le met à ff
    eeprom_write_byte((uint8_t*)0, 0xff);

    // Effacer le reste (1 à 1023) en fixant à 0x00
    for (uint16_t addr = 0; addr < EEPROM_MAX_SIZE; addr++) {
        eeprom_write_byte((uint8_t*)addr, 0x00);
    }

    UART__putc(STATUS_OK);
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
        }
    }
    return 0;
}