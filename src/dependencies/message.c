#ifndef __MESSAGE__
#define MESSAGE

#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "header.h"
#include "message.h"

/*  Fonction de formalisation de message
    *var ip :       adresse ip de l'expediteur
    *var dest :     pseudo du destinataire
    *var msg :      Message a envoyer 

    *return : message formalise ("ip//dest//numero msg//nb total message/#/#00message")
*/
char *createMessage(char *ip, char *dest, char *msg)
{
    char *message = malloc(1024*sizeof(char));

    memset(message, 0, BUFFER_MAX);

    strcpy(message, ip);
    strcat(message, "//");

    strcat(message, dest);
    strcat(message, "//");

    strcat(message, "1");
    strcat(message, "//");

    strcat(message, "1");
    strcat(message, "/#/#00");

    strcat(message, msg);

    return message;
}

// Fonction qui parse le buffer en 5 parties : ip, pseudo, num, total, payload
int parseMessage(const char *buffer, MessageInfo *msg) {
    const char *p = buffer, *sep;
    size_t len;
    char tmp[16];

    // 1) Extraction de l'adresse IP de l'expéditeur
    sep = strstr(p, FIELD_DELIM);
    if (!sep) return -1;
    len = sep - p;
    memcpy(msg->sender_ip, p, len);
    msg->sender_ip[len] = '\0';

    // 2) Extraction du pseudo du destinataire
    p = sep + strlen(FIELD_DELIM);
    sep = strstr(p, FIELD_DELIM);
    if (!sep) return -1;
    len = sep - p;
    memcpy(msg->dest_pseudo, p, len);
    msg->dest_pseudo[len] = '\0';

    // 3) Extraction du numéro de la partie du message
    p = sep + strlen(FIELD_DELIM);
    sep = strstr(p, FIELD_DELIM);
    if (!sep) return -1;
    len = sep - p;
    memcpy(tmp, p, len); tmp[len] = '\0';
    msg->part_num = atoi(tmp); // conversion en entier

    // 4) Extraction du nombre total de parties
    p = sep + strlen(FIELD_DELIM);
    sep = strstr(p, PAYLOAD_DELIM);     
    if (!sep) return -1;
    len = sep - p;
    memcpy(tmp, p, len); tmp[len] = '\0';
    msg->total_parts = atoi(tmp);

    // 5) Extraction du contenu (payload)
    p = sep + strlen(PAYLOAD_DELIM);
    strncpy(msg->payload, p, sizeof(msg->payload)-1);
    msg->payload[sizeof(msg->payload)-1] = '\0';

    return 0; 
}

int main()
{
    return 0;
}
#endif