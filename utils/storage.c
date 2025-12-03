#include <avr/io.h>
#include <avr/eeprom.h>
#include <stdint.h>

#include "constants.h"
#include "consent.h"
#include "storage.h"

// cette fonction recherche dans l'eeprom une entree qui correspond au app_id_hash mis en parametre
SearchResult search__by__app_id_hash (const uint8_t app_id_hash[20]){
    SearchResult out;
    // On consulte le nombre de clés actuellement stockées, cette valeur est conservée l'offset 0   
    out.nb_entries = eeprom_read_byte((uint8_t*)0);

    // Si l'EEPROM à été vidée (réinitialisation de la carte ou nouveau téléversage par exemple), elle contient 0xff
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

// on ecrit dans l'eeprom le tuple passé en parametre
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

// cherche si une certaine entree existe (en appelant la fonction search__by__app_id_hash). Le cas échéant, elle stocke les valeurs credential_id et private_key dans les pointeurs fournis en parametres
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