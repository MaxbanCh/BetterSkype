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
int associateUser(const char *pseudo, const struct sockaddr_in *client, User *activeUsers, int *numActiveUsers) {
    // Récupération de l'IP et du port du client
    char clientIp[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client->sin_addr), clientIp, INET_ADDRSTRLEN);
    int clientPort = ntohs(client->sin_port);
    
    // Recherche si l'utilisateur existe déjà ou trouver un emplacement libre
    int index = -1;
    int existingUserIndex = -1;
    int freeIndex = -1;
    int i = 0;
    
    while (i < MAX_USERS) {
        // Si on trouve l'utilisateur avec le même pseudo
        if (strcmp(activeUsers[i].pseudo, pseudo) == 0) {
            existingUserIndex = i;
        }
        // Si on trouve un emplacement libre
        if (activeUsers[i].pseudo[0] == '\0' && freeIndex == -1) {
            freeIndex = i;
        }
        i++;
    }
    
    // On privilégie l'utilisateur existant, sinon on prend un emplacement libre
    if (existingUserIndex != -1) {
        index = existingUserIndex;
    } else if (freeIndex != -1) {
        index = freeIndex;
        (*numActiveUsers)++;
    }
    
    // Si on a trouvé un index valide, on met à jour les données
    if (index != -1) {
        strncpy(activeUsers[index].pseudo, pseudo, PSEUDO_MAX - 1);
        strncpy(activeUsers[index].ip, clientIp, INET_ADDRSTRLEN - 1);
        activeUsers[index].port = clientPort;
        activeUsers[index].isConnected = 1;
        
        // Vérifier si c'est le premier utilisateur connecté
        int anyOtherConnected = 0;
        i = 0;
        
        while (i < MAX_USERS) {
            if (i != index && activeUsers[i].isConnected == 1) {
                anyOtherConnected = 1;
            }
            i++;
        }
        
        // Si aucun autre utilisateur n'est connecté, c'est l'administrateur
        if (anyOtherConnected == 0) {
            activeUsers[index].isAdmin = 1;
            printf("L'utilisateur %s est l'administrateur\n", pseudo);
        } else {
            activeUsers[index].isAdmin = 0;
            printf("L'utilisateur %s n'est pas administrateur\n", pseudo);
        }
        }
    
    int result = 0;
    if (index != -1) {
        result = 1;
    }
    
    return result;
}
