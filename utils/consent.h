#ifndef CONSENT_H
#define CONSENT_H

#include <stdint.h>

/*
 * Fonction qui attend le consentement utilisateur.
 * Retourne :
 *   - 1 si l'utilisateur a appuyé sur le bouton avant le timeout
 *   - 0 si le timeout est expiré
 *
 * Le comportement exact (réel ou bypass) dépend de BYPASS_USER_CONSENT
 * défini dans config.h.
 */
uint8_t wait_for_user_consent(void);

#endif