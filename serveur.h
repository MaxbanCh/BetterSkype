#ifndef SERVER_H
#define SERVER_H

#include <sys/types.h> 
#include <sys/socket.h>     
#include <netinet/in.h>     
#include <stddef.h>         
#include "MessageInfo.h"   

// Initialise et bind une socket UDP sur le port défini
int   initSocket(void);

// Reçoit un message depuis un client UDP
ssize_t recvMessage(int sockfd, char *buffer, size_t buflen, struct sockaddr_in *cli);

// Parse buffer en champs sender_ip, dest_pseudo, part_num, total_parts, payload
int   parseMessage(const char *buffer, MessageInfo *msg);

// Affiche le message reçu
void  printMessage(const MessageInfo *msg);

// Envoie un accusé de réception  à l’adresse client
void  sendAcR(int sockfd, const struct sockaddr_in *cli);

#endif 