#ifndef COMMAND_H
#define COMMAND_H
#include <user.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#endif
#include <stddef.h>
#include <header.h>
#include <stdio.h>  

/// Toutes les commandes supportées
typedef enum {
    cmdConnect,
    cmdDisconnect,
    cmdRegister,
    cmdMsg,
    cmdHelp,
    cmdPing,
    cmdCredits,
    cmdShutdown,
    cmdUpload,
    cmdDownload,
    cmdUnknown
} CommandType;


// Renvoie le type de commande détecté au début de payload.
CommandType getCommandType(const char *payload);

// Traite la commande @connect
int connectCmd(const char *payload, const struct sockaddr_in *client, char *response, size_t response_size, User *activeUsers, int *numActiveUsers);

// Fonction qui enregistre un nouvel utilisateur
int registerUser(const char *payload, const struct sockaddr_in *client, char *response, size_t response_size, User *activeUsers, int *numActiveUsers);

// Traite la commande @disconnect
int disconnectCmd(const char *payload, const struct sockaddr_in *client, char *response, size_t response_size, User *activeUsers, int numActiveUsers);

// Traite la commande @msg pour envoyer un message privé
int sendPrivateMsg(const char *payload, const struct sockaddr_in *sender_client, char *response, size_t response_size, User *activeUsers, int numActiveUsers);

int helpCmd(const char *payload, const struct sockaddr_in *client, char *response, size_t response_size, User *activeUsers, int numActiveUsers);

int pingCmd(const char *payload, const struct sockaddr_in *client, char *response, size_t response_size, User *activeUsers, int numActiveUsers);

int creditsCmd(const char *payload, const struct sockaddr_in *client, char *response, size_t response_size, User *activeUsers, int numActiveUsers);

int shutdownCmd(const char *payload, const struct sockaddr_in *client, char *response, size_t response_size, User *activeUsers, int numActiveUsers);

#endif // COMMAND_H