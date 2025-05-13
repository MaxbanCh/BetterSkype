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

// Fonction d'enregistrement d'un nouvel utilisateur
int registerUser(const char *pseudo, const char *password) {
    // Vérifier si l'utilisateur existe déjà
    int auth_result = authenticateClient(pseudo, "");
    if (auth_result == 1 || auth_result == -1) {
        return 0; // L'utilisateur existe déjà
    }

    // Ajouter le nouvel utilisateur au fichier
    FILE *file = fopen(USERS_FILE, "a");
    if (!file) {
        file = fopen(USERS_FILE, "w"); // Créer s'il n'existe pas
        if (!file) {
            perror("Impossible de créer le fichier utilisateurs");
            return -1;
        }
    }

    // Dans une implémentation réelle, on hasherait le mot de passe ici
    fprintf(file, "%s,%s\n", pseudo, password);
    fclose(file);
    return 1; // Enregistrement réussi
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

// Traitement de la commande @connect
int connectCmd(const char *payload, const struct sockaddr_in *client, char *response, size_t response_size, User *activeUsers, int *numActiveUsers) {
    char pseudo[PSEUDO_MAX];
    char password[64];
    
    // Extraction du login et mot de passe
    // Format: "@connect login mdp"
    if (sscanf(payload, "@connect %s %s", pseudo, password) != 2) {
        snprintf(response, response_size, "Format invalide. Utilisez: @connect login mdp");
        return 0;
    }
    
    // Vérifier si l'utilisateur est déjà connecté
    if (isUserConnected(pseudo, activeUsers, *numActiveUsers)) {
        snprintf(response, response_size, "L'utilisateur %s est déjà connecté", pseudo);
        return 0;
    }
    
    // Authentifier l'utilisateur
    int auth_result = authenticateClient(pseudo, password);
      if (auth_result == 1) {
        // Authentification réussie
        if (associateUser(pseudo, client, activeUsers, numActiveUsers)) {
            snprintf(response, response_size, "Connecté en tant que %s", pseudo);
            return 1;
        } else {
            snprintf(response, response_size, "Erreur serveur: impossible d'ajouter l'utilisateur à la liste active");
            return 0;
        }
    } else if (auth_result == -1) {
        // Mot de passe incorrect
        snprintf(response, response_size, "Mot de passe invalide pour l'utilisateur %s", pseudo);
        return 0;
    } else {        // L'utilisateur n'existe pas, essayer de l'enregistrer
        if (registerUser(pseudo, password)) {
            // Enregistrement réussi
            if (associateUser(pseudo, client, activeUsers, numActiveUsers)) {
                snprintf(response, response_size, "Enregistré et connecté en tant que %s", pseudo);
                return 1;
            } else {
                snprintf(response, response_size, "Erreur serveur: impossible d'ajouter l'utilisateur à la liste active");
                return 0;
            }        } else {
            snprintf(response, response_size, "Échec de l'enregistrement de l'utilisateur %s", pseudo);
            return 0;
        }
    }
    
    // Si on arrive ici, quelque chose d'inattendu s'est produit
    snprintf(response, response_size, "Erreur inconnue lors de l'authentification");
    return 0;
}

// Traite la commande @disconnect
int disconnectCmd(const char *payload, const struct sockaddr_in *client, char *response, size_t response_size, User *activeUsers, int numActiveUsers) {
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client->sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client->sin_port);
    
    // Chercher l'utilisateur avec cette IP et port
    for (int i = 0; i < numActiveUsers; i++) {
        if (strcmp(activeUsers[i].ip, client_ip) == 0 && 
            activeUsers[i].port == client_port && 
            activeUsers[i].isConnected == 1) {
            
            // Déconnecter l'utilisateur
            activeUsers[i].isConnected = 0;
            snprintf(response, response_size, "Déconnecté: %s", activeUsers[i].pseudo);
            return 1;
        }
    }
    
    snprintf(response, response_size, "Vous n'êtes pas connecté");
    return 0;
}