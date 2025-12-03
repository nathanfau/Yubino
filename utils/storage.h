#ifndef STORAGE_H
#define STORAGE_H


// on a discuté de ces valeurs dans le README
#define ENTRY_SIZE 57
#define EEPROM_MAX_SIZE 1024
#define MAX_ENTRIES ((EEPROM_MAX_SIZE - 1) / ENTRY_SIZE)


// on définit ici une struct qui est le type renvoyé par la fonction search__by__app_id_hash
// Ceci nous permet de renvoyer plusieurs valeurs à la fois lors d'un return sans avoir à utiliser de tableau ou de passer les valeurs de retour en parametre (via pointeurs)
typedef struct {
    uint8_t nb_entries;
    uint8_t match;
    uint16_t index;
} SearchResult;

SearchResult search__by__app_id_hash (const uint8_t app_id_hash[20]);
uint8_t store__credential__in__eeprom(const uint8_t app_id_hash[20], const uint8_t credential_id[16], const uint8_t private_key[21]);
uint8_t storage__search(const uint8_t *app_id_hash, uint8_t *credential_id, uint8_t *private_key);


#endif