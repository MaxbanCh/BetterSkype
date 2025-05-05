#ifndef SERVER_H
#define SERVER_H

#include <sys/types.h> 
#include <sys/socket.h>     
#include <netinet/in.h>     
#include <stddef.h>         
#include "MessageInfo.h"   

// Initialise et bind une socket UDP sur le port défini
int   initSocket(void);

// Reçoit un datagramme UDP sans dépasser buflen-1, termine buffer par '\0'
ssize_t recvMessage(int sockfd, char *buffer, size_t buflen, struct sockaddr_in *cli);

// Parse buffer en champs sender_ip, dest_pseudo, part_num, total_parts, payload
int   parseMessage(const char *buffer, MessageInfo *msg);

// Affiche joliment le contenu de msg
void  printMessage(const MessageInfo *msg);

// Envoie un accusé de réception (ACK) à l’adresse cli
void  sendAcR(int sockfd, const struct sockaddr_in *cli);

#endif 