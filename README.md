**Nathan FAUVELLE-AYMAR (MIC) & Jonas DOS SANTOS (MIC)**

## Ce que fait le programme
Ce programme permet d'implémenter sur carte Arduino UNO R3 un programme permettant à la carte de jouer un rôle **d'Authenticator** dans le protocole FIDO2. 


## Comment l'exécuter
Effectuer la commande suivante depuis le dossier dans lequel apparaît le fichier main.c pour générer le .elf, générer le .hex et flasher le code sur la carte :
```bash
make
```

Si on souhaite ignorer les interactions avec l'utilisateur (à savoir appuyer sur le bouton) pour les valider automatiquement, effectuer la commande :
```bash
make bypass
```

Pour effectuer les tests, il est recommandé de suivre les instructions données dans le fichier ./yubino-client/README.md fourni avec le sujet du projet.
Il est à noter que pour effectuer la batterie de tests fournie, et tester fiablement la rapidité d'exécution, la commande **make bypass** est recommandée.


## ORGANISATION DU CODE
Vous trouverez dans ce dossier les éléments suivants :
- **micro-ecc** la bibliothèque permettant les opérations cryptographiques avec l'algorithme ECDSA.
- **yubino-client** le dossier fourni avec le sujet permettant de run **yubino** et effectuer l'ensemble des tests.
- **utils** qui comprend l'ensemble des fichiers .h et .c utiles à l'ATMega328P. Les rôles de chaque fichiers sont en général explicités par leurs noms, et certains sont directement récupérés des TPs qui ont été effectué durant le semestre.
- **main.c** le fichier contenant le main de notre code.
- **Makefile** le fichier permettant de flasher le code sur la carte selon les instructions précédentes.

## REMARQUES GENERALES


## GESTION DE LA MEMOIRE
- Le microcontrôleur ATmega328P dispose de **2 Ko de SRAM** et **1 Ko d’EEPROM**, soit **1024 octets d'EEPROM**.

- Les credentials FIDO sont stockés en EEPROM dans un format compact :
  - 20 octets : SHA1(app_id)
  - 16 octets : Credential ID
  - 21 octets : clé privée ECC
soit **57 octets** par entrée, stockés séquentiellement.

- L’adresse EEPROM 0 contient le **nombre d’entrées** enregistrées, qui sera donc au maximum de 1024/57 = 17. Une tentative d'enregistrer un credential supplémentaire (or écrasement d'un déjà existant) lors que le nombre d'entrées maximum est atteint renvoie un **error code 5 : STATUS_ERR_STORAGE_FULL**.

- Aucune allocation dynamique (`malloc`) n’est utilisée : toutes les données sont gérées via
  des buffers de **taille fixe et minimale** (hash 20 octets, clés 21/40 octets, etc.).

- Seules les données persistantes sont écrites en EEPROM ; toutes les autres sont stockées temporairement en SRAM et disparaissent automatiquement.


## GESTION DE L'ENERGIE



## IMPLEMENTATION DU RANDOM



## UART ET RINGBUFFER (+ bouton et allumage de LED)
Les fonctions **uart.h**, **uart.c**, **ringbuffer.h**, **ringbuffer.c** sont tirées du **TP4**.
L'implémentation du bouton est tirée du **TP5**, et la méthode d'allumage de la LED est tirée du **TP3**.


## GESTION DU CONSENTEMENT (interaction ou non avec l'utilisateur)
- L’authentificateur doit, selon le sujet, demander un **consentement explicite** de l’utilisateur
  (appui sur un bouton) avant de réaliser certaines opérations.

- Le comportement est contrôlé par le flag de compilation **BYPASS_USER_CONSENT**, défini
  dans le Makefile :
  - `0` : consentement réel (LED clignotante + pression bouton + timeout).
  - `1` : mode bypass, le consentement est automatiquement validé.

- La fonction `wait_for_user_consent()` gère :
  - le clignotement de la LED,
  - l’attente du bouton,
  - un timeout d’environ 10 secondes,
  - ou un retour immédiat si le bypass est activé.

- Cela permet de réaliser des tests automatisés (notamment avec `make bypass`)
  sans interaction physique avec la carte.

