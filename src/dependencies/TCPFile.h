#ifndef TCPFILE_H
#define TCPFILE_H

#include "message.h"

typedef struct {
    char *operation;
    char *filename;
    struct sockaddr_in client;
    int socketTCP;
} FileTransferParams;


int initTCPSocketServer();
int initTCPSocketClient(char *ip);
int connexionTCP(int socket);

int sendFile(int socket, const char *file);
int receiveFile(int socket, const char *file);

void *fileTransferThreadServer(void *arg);

void closeServer(int socket, int socketClient);
void closeClient(int socket);

#endif