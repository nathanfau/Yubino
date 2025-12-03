**Nathan FAUVELLE-AYMAR (MIC) & Jonas DOS SANTOS (MIC)**

## Ce que fait le programme
Ce programme permet d'implémenter sur carte Arduino UNO R3 un programme permettant à la carte de jouer un rôle **d'Authenticator** dans un protocole de type FIDO2.


## Comment l'exécuter
Effectuer la commande suivante depuis le dossier dans lequel apparaît le fichier main.c pour générer le .elf, générer le .hex et flasher le code sur la carte :
```bash
make
```

Si on souhaite ignorer les interactions avec l'utilisateur (à savoir appuyer sur le bouton) pour les valider automatiquement, effectuer la commande :
```bash
make bypass
```

Veuillez penser à **clean** avant de changer le mode d'interactions (consentement/automatique)
```bash
make clean
```

Pour effectuer les tests, il est recommandé de suivre les instructions données dans le fichier ./yubino-client/README.md fourni avec le sujet du projet.
Il est à noter que pour effectuer la batterie de tests fournie, et tester fiablement la rapidité d'exécution, la commande **make bypass** est recommandée.


## ORGANISATION DU CODE
Vous trouverez dans ce dossier les éléments suivants :
- **schema/** contenant un fichier au format pdf et une fichier au format png réprésentant le montage nécessaire pour intégrer le bouton. (ces fichiers ont été générés à l'aide de l'outil en ligne "tinkercad")
- **micro-ecc/** la bibliothèque permettant les opérations cryptographiques avec l'algorithme ECDSA.
- **yubino-client/** le dossier fourni avec le sujet permettant de run **yubino** et effectuer l'ensemble des tests.
- **utils/** qui comprend l'ensemble des fichiers .h et .c utiles à l'ATMega328P. Les rôles de chaque fichiers sont en général explicités par leurs noms, et certains sont directement récupérés des TPs qui ont été effectué durant le semestre.
- **main.c** le fichier contenant le main de notre code.
- **Makefile** le fichier permettant de flasher le code sur la carte selon les instructions précédentes.

## REMARQUES GENERALES

Un souci rencontré a été la gestion du **baud rate**, le sujet imposait de définir la valeur du **baud rate** à _115 200_, ce qui générait au départ, énormemément d'erreurs de transmission.
Nous nous sommes donc penché sur la question est sommes allés lire la doc [1], les sections **20.8** , **20.9** et **20.10** se sont révélés très intéressantes et nous ont permis de comprendre qu'il nous fallait utiliser le _Double Speed Mode_, ce qui permet, lorsqu'un CPU tourne à 16Mhz et qu'on configure le baud rate à 115 200 d'avoir au maximum un taux d'erreurs de **2.1%**. 

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
Le microcontrôleur dort la majeur partie de son temps.
Nous avons utilisé le mode **SLEEP_MODE_IDLE** qui est le sommeil le plus _léger_.
Lorsque le microcontrôleur s'endort avec ce mode, l'**UART**, les **timers**, le **watch dog** et l'**ADC** continuent de fonctionner.

Le microcontrôleur s'endort dans 2 cas:
- S'il ne reçoit rien via l'**UART** et qu'il n'est pas en pleine action
- S'il demande à l'utilisateur son consentement (sommeil < 10s)

Dans les 2 cas ci-dessus, le microcontrôleur doit se réveiller si:
- Il reçoit un nouveau message via l'**UART**, on doit donc garder celui-ci _allumé_.
- L'utilisateur appuie sur le bouton, ce qui est détecté par le **watch dog**, on doit donc le garder _allumé_.
- L'utilisateur n'a pas donné son consentement en 10s, ce qui est détecté par le **timer1**, on doit donc le garder _allumé_.

Le mode **IDLE** est le seul mode de sommeil qui laisse **clk_IO** _allumé_, on ne peut donc utiliser que celui-ci.

Nous avons pensé à désactiver par moment les periphériques non utilisés grâce au **Power Reduction Register** mais il nous a semblé que modifier les bits de ce registre très souvent n'est pas forcément rentable. La question a se poser est : Allumer/eteindre certains périphériques coûte-t-il plus cher que ce que nous rapporte ces quelques secondes (ou moins) de _power reduction_ ?

## IMPLEMENTATION DE L'ALEA
La bibliothèque micro-ecc nécessite une source d'aléa, nous avons décidé d'implémenter celle-ci comme suit:
- **random__init()** initialise l'ADC et le timer0 avant de commencer à générer de l'aléa.
- **random__get(_dest_, _size_)** génère _size_ octets aléatoire et les enregistre à l'adresse de _dest_.

Pour générer 1 octet aléatoire, on génère 8 bits aléatoirement.
Chacun de ces bits est généré en combinant la valeur du bit de poids faible du timer0 (ce bit change très souvent car notre timer tourne très vite) et le bit de poids faible de l'ADC (bruit thermique, ce bit et lui aussi imprévisible).

## UART ET RINGBUFFER (+ bouton et allumage de LED)
Les fonctions **uart.h**, **uart.c**, **ringbuffer.h**, **ringbuffer.c** sont tirées du **TP4**.
L'implémentation du bouton est tirée du **TP5**, et la méthode d'allumage de la LED est tirée du **TP3**.

Pour le bouton, nous avons implémenté l'algorithme de debounce proposé dans le **TP5** et utilisé le **Watchdog Timer**.


## GESTION DU CONSENTEMENT (interaction ou non avec l'utilisateur)
- L’authentificateur doit, selon le sujet, demander un **consentement explicite** de l’utilisateur
  (appui sur un bouton) avant de réaliser certaines opérations.

- Le comportement est contrôlé par le flag de compilation **BYPASS_USER_CONSENT**, défini
  dans le Makefile :
  - `0` : consentement réel (LED clignotante + pression bouton + timeout).
  - `1` : mode bypass, le consentement est automatiquement validé.

- La fonction `wait__for__user__consent()` gère :
  - le clignotement de la LED,
  - l’attente du bouton,
  - un timeout de 10 secondes,
  - ou un retour immédiat si le bypass est activé.

- Cela permet de réaliser des tests automatisés (notamment avec `make bypass`)
  sans interaction physique avec la carte.

## Références 

[[1] - Documentation ATmega](https://ww1.microchip.com/downloads/en/DeviceDoc/ATmega48A-PA-88A-PA-168A-PA-328-P-DS-DS40002061A.pdf)