#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "header.h"
#include "user.h"


// Fonction qui vérifie l'authentification d'un client
int authenticateClient(const char *pseudo, const char *password) {
    FILE *file = fopen(USERS_FILE, "r");
    if (!file) {
        // Premier utilisateur, le fichier n'existe pas encore
        return 0;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char storedPseudo[PSEUDO_MAX];
        char storedPassword[64];
        
        // Analyse de la ligne CSV
        if (sscanf(line, "%[^,],%[^,\n]", storedPseudo, storedPassword) == 2) {
            if (strcmp(storedPseudo, pseudo) == 0) {
                // Utilisateur trouvé, vérifier le mot de passe
                // Dans une implémentation réelle, on hasherait le mot de passe
                if (strcmp(storedPassword, password) == 0) {
                    fclose(file);
                    return 1; // Authentification réussie
                }
                fclose(file);
                return -1; // Mot de passe incorrect
            }
        }
    }
    fclose(file);
    return 0; // Utilisateur non trouvé
}



// Vérifie si un utilisateur est déjà connecté
int isUserConnected(const char *pseudo, User *activeUsers, int numActiveUsers) {
    for (int i = 0; i < numActiveUsers; i++) {
        if (strcmp(activeUsers[i].pseudo, pseudo) == 0 && activeUsers[i].isConnected) {
            return 1; // Utilisateur connecté
        }
    }
    return 0; // Utilisateur non connecté
}

// Associe un utilisateur à une adresse IP et port
int associateUser(const char *pseudo, const struct sockaddr_in *client_addr, User *activeUsers, int *numActiveUsers) {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr->sin_addr), ip, INET_ADDRSTRLEN);
    int port = ntohs(client_addr->sin_port);
    
    // Vérifier si l'utilisateur est déjà dans la liste
    for (int i = 0; i < *numActiveUsers; i++) {
        if (strcmp(activeUsers[i].pseudo, pseudo) == 0) {
            // Mettre à jour l'utilisateur existant
            strcpy(activeUsers[i].ip, ip);
            activeUsers[i].port = port;
            activeUsers[i].isConnected = 1;
            return 1;
        }
    }
    
    // Ajouter un nouvel utilisateur actif
    if (*numActiveUsers >= MAX_USERS) {
        return 0; // Liste pleine
    }
    
    strcpy(activeUsers[*numActiveUsers].pseudo, pseudo);
    strcpy(activeUsers[*numActiveUsers].ip, ip);
    activeUsers[*numActiveUsers].port = port;
    activeUsers[*numActiveUsers].isConnected = 1;
    (*numActiveUsers)++;
    return 1;
}

