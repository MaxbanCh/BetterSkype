// #include <string.h>
// #include <stdlib.h>
// #include <arpa/inet.h>

#ifndef __MESSAGEH__
#define __MESSAGEH__
// si ces constantes ne sont pas définies ailleurs, on les fixe ici :
#ifndef PSEUDO_MAX
#define PSEUDO_MAX 50
#endif

#ifndef BUFFER_MAX
#define BUFFER_MAX 1024
#endif

typedef struct {
    char sender_ip[INET_ADDRSTRLEN];
    char dest_pseudo[PSEUDO_MAX];
    int  part_num;
    int  total_parts;
    char payload[BUFFER_MAX];
} MessageInfo;


/*  Fonction de formalisation de message
    *var ip :       adresse ip de l'expediteur
    *var dest :     pseudo du destinataire
    *var msg :      Message a envoyer 

    *return : message formalise ("ip//dest//numero msg//nb total message/#/#00message")
*/
char *createMessage(char *ip, char *dest, char *msg);

// Fonction qui parse le buffer en 5 parties : ip, pseudo, num, total, payload
int parseMessage(const char *buffer, MessageInfo *msg);

// Fonction d'affichage d'un message
void printMessage(const MessageInfo *m);
#endif