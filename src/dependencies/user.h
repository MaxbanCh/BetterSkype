#include <arpa/inet.h> 
#include <header.h>

#ifndef USER_H
#define USER_H

// Structure pour stocker les informations utilisateur
typedef struct {
    char pseudo[PSEUDO_MAX]; // Pseudo de l'utilisateur
    char password[64]; // Mot de passe de l'utilisateur
    char ip[INET_ADDRSTRLEN]; // Adresse IP de l'utilisateur
    int port; // Port de l'utilisateur
    int isConnected; 
} User;

// Vérifie l'authentification d'un client
int authenticateClient(const char *pseudo, const char *password);

// Associe un pseudo à une adresse IP + port
int associateUser(const char *pseudo, const struct sockaddr_in *client_addr, User *activeUsers, int *numActiveUsers);

// Vérifie si un utilisateur est connecté
int isUserConnected(const char *pseudo, User *activeUsers, int numActiveUsers);

#endif // USER_H