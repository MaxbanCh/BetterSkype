#ifndef TCPFILE_H
#define TCPFILE_H

#include "message.h"

int initTCPSocketServer();
int initTCPSocketClient(char *ip);
int connexionTCP(int socket);

int sendFile(int socket, const char *file);
int receiveFile(int socket, char *file);

void closeServer(int socket, int socketClient);
void closeClient(int socket);

#endif