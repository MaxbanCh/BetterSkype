#include <string.h>
#include "command.h"
#include <stdio.h>
#include <header.h>
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <arpa/inet.h>
    #include <netinet/in.h>
#endif



CommandType getCommandType(const char *payload)
{
    CommandType cmdType = cmdUnknown;

    // on compare juste le préfixe
    if (strncmp(payload, "@connect",    8) == 0) {
        cmdType = cmdConnect;
    }
    else if (strncmp(payload, "@disconnect", 11) == 0) {
        cmdType = cmdDisconnect;
    }
    else if (strncmp(payload, "@register",  9) == 0) {
        cmdType = cmdRegister;
    }
    else if (strncmp(payload, "@msg",       4) == 0) {
        cmdType = cmdMsg;
    }
    else if (strncmp(payload, "@help",      5) == 0) {
        cmdType = cmdHelp;
    }
    else if (strncmp(payload, "@ping",      5) == 0) {
        cmdType = cmdPing;
    }
    else if (strncmp(payload, "@credits",   8) == 0) {
        cmdType = cmdCredits;
    }
    else if (strncmp(payload, "@shutdown",  9) == 0) {
        cmdType = cmdShutdown;
    }
    else if (strncmp(payload, "@upload",    7) == 0) {
        cmdType = cmdUpload;
    }
    else if (strncmp(payload, "@download",  9) == 0) {
        cmdType = cmdDownload;
    }

    return cmdType;
}


int pingCmd(const char *payload, const struct sockaddr_in *client, 
           char *response, size_t response_size, User *activeUsers, int numActiveUsers) {
    
    // Get client information
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client->sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client->sin_port);
    
    // Check if user is connected
    int isConnected = 0;
    char userName[PSEUDO_MAX] = "Anonyme";
    int i = 0;
    
    while (i < numActiveUsers && !isConnected) {
        if (strcmp(activeUsers[i].ip, client_ip) == 0 && 
            activeUsers[i].port == client_port && 
            activeUsers[i].isConnected == 1) {
            isConnected = 1;
            strncpy(userName, activeUsers[i].pseudo, PSEUDO_MAX);
        }
        i++;
    }
    
    // Format a personalized response
    if (isConnected) {
        snprintf(response, response_size, "Pong! Serveur en ligne. Vous êtes connecté en tant que %s.", userName);
    } else {
        snprintf(response, response_size, "Pong! Serveur en ligne. Vous n'êtes pas connecté.");
    }
    
    return 1; // Success - ping response created successfully
}

// Fonction d'enregistrement d'un nouvel utilisateur
int registerUser(const char *payload, const struct sockaddr_in *client, char *response, size_t response_size, User *activeUsers, int *numActiveUsers) {
    char pseudo[PSEUDO_MAX];
    char password[64];
    
    // Extraire le pseudo et le mot de passe du payload
    // Format: "@register pseudo password"
    if (sscanf(payload, "@register %s %s", pseudo, password) != 2) {
        snprintf(response, response_size, "Format invalide. Utilisez: @register <pseudo> <password>");
        return 0;
    }
    
    // Vérifier si l'utilisateur existe déjà
    int auth_result = authenticateClient(pseudo, "");
    if (auth_result == 1 || auth_result == -1) {
        snprintf(response, response_size, "Le pseudo %s est déjà utilisé", pseudo);
        return 0; // L'utilisateur existe déjà
    }

    // Ajouter le nouvel utilisateur au fichier
    FILE *file = fopen(USERS_FILE, "a");
    if (!file) {
        file = fopen(USERS_FILE, "w"); // Créer s'il n'existe pas
        if (!file) {
            perror("Impossible de créer le fichier utilisateurs");
            snprintf(response, response_size, "Erreur serveur lors de la création du fichier utilisateurs");
            return 0;
        }
    }

    // Dans une implémentation réelle, on hasherait le mot de passe ici
    fprintf(file, "%s,%s\n", pseudo, password);
    fclose(file);
    
    // Associer l'utilisateur à son adresse IP et port
    if (associateUser(pseudo, client, activeUsers, numActiveUsers)) {
        snprintf(response, response_size, "Utilisateur %s enregistré et connecté avec succès", pseudo);
        return 1;
    } else {
        snprintf(response, response_size, "Utilisateur %s enregistré mais erreur lors de la connexion", pseudo);
        return 0;
    }
}

// Traitement de la commande @connect
int connectCmd(const char *payload, const struct sockaddr_in *client, char *response, size_t response_size, User *activeUsers, int *numActiveUsers) {
    char pseudo[PSEUDO_MAX];
    char password[64];
    
    // Extraction du login et mot de passe
    if (sscanf(payload, "@connect %s %s", pseudo, password) != 2) {
        snprintf(response, response_size, "Format invalide. Utilisez: @connect login mdp");
        return 0;
    }
    
    char clientIp[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client->sin_addr), clientIp, INET_ADDRSTRLEN);
    int clientPort = ntohs(client->sin_port);
    
    
    // Chercher si un utilisateur est déjà connecté depuis cette adresse
    for (int i = 0; i < *numActiveUsers; i++) {
        if (strcmp(activeUsers[i].ip, clientIp) == 0 && 
            activeUsers[i].port == clientPort && 
            activeUsers[i].isConnected == 1) {
            
            // Si c'est le même utilisateur qui essaie de se reconnecter
            if (strcmp(activeUsers[i].pseudo, pseudo) == 0) {
                snprintf(response, response_size, "Vous êtes déjà connecté en tant que %s", pseudo);
                return 0; // Empêcher la reconnexion inutile
            }
            
            // Sinon, déconnecter l'utilisateur existant
            printf("Déconnexion de %s (même client tente une nouvelle connexion)\n", 
                   activeUsers[i].pseudo);
            activeUsers[i].isConnected = 0;
        }
    }
    
    // Vérifier si l'utilisateur est déjà connecté depuis un autre client
    if (isUserConnected(pseudo, activeUsers, *numActiveUsers)) {
        snprintf(response, response_size, "L'utilisateur %s est déjà connecté depuis un autre terminal", pseudo);
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
        return 0;    } else {        
        // L'utilisateur n'existe pas, l'enregistrer directement
        // Créer un payload pour l'enregistrement
        char register_payload[BUFFER_MAX];
        snprintf(register_payload, sizeof(register_payload), "@register %s %s", pseudo, password);
        
        // Utiliser la nouvelle version de registerUser
        if (registerUser(register_payload, client, response, response_size, activeUsers, numActiveUsers)) {
            return 1; // L'utilisateur est déjà enregistré et connecté par registerUser
        } else {
            // registerUser a déjà défini la réponse
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

int sendPrivateMsg(const char *payload, const struct sockaddr_in *senderClient, 
                  char *response, size_t response_size, User *activeUsers, int numActiveUsers) {
    // Variables pour suivre l'état de la fonction
    int senderIndex = -1;
    int destIndex = -1;
    char destPseudo[PSEUDO_MAX];
    memset(destPseudo, 0, sizeof(destPseudo));
    char *msgStart;
    char *contentStart;
    
    // Vérifier que l'expéditeur est connecté
    char senderIp[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(senderClient->sin_addr), senderIp, INET_ADDRSTRLEN);
    int senderPort = ntohs(senderClient->sin_port);
    
    // Trouver l'expéditeur
    int i = 0;
    while (i < numActiveUsers && senderIndex == -1) {
        if (strcmp(activeUsers[i].ip, senderIp) == 0 && 
            activeUsers[i].port == senderPort && 
            activeUsers[i].isConnected == 1) {
            senderIndex = i;
        }
        i++;
    }
    
    // État de traitement du message - défini APRÈS la vérification de l'expéditeur
    enum {
        CHECK_SENDER,
        EXTRACT_DESTINATION,
        EXTRACT_MESSAGE,
        FIND_DESTINATION,
        FORMAT_MESSAGE,
        MSG_ERROR_NOT_CONNECTED,
        ERROR_INVALID_FORMAT,
        ERROR_MISSING_MESSAGE,
        ERROR_USER_NOT_FOUND
    } state;
    
    // Initialiser l'état en fonction de si l'expéditeur est trouvé
    if (senderIndex == -1) {
        state = MSG_ERROR_NOT_CONNECTED;
    } else {
        // Expéditeur trouvé, extraire le destinataire
        msgStart = strchr(payload, ' ');
        if (!msgStart) {
            state = ERROR_INVALID_FORMAT;
        } else {
            msgStart++; // Sauter l'espace
            
            if (sscanf(msgStart, "%s", destPseudo) != 1) {
                state = ERROR_INVALID_FORMAT;
            } else {
                // Trouver le message (tout ce qui vient après le pseudo)
                contentStart = strchr(msgStart, ' ');
                if (!contentStart) {
                    state = ERROR_MISSING_MESSAGE;
                } else {
                    contentStart++; // Sauter l'espace
                    
                    // Trouver le destinataire
                    i = 0;
                    while (i < numActiveUsers && destIndex == -1) {
                        if (strcmp(activeUsers[i].pseudo, destPseudo) == 0 && 
                            activeUsers[i].isConnected == 1) {
                            destIndex = i;
                        }
                        i++;
                    }
                    
                    if (destIndex != -1) {
                        state = FORMAT_MESSAGE;
                    } else {
                        state = ERROR_USER_NOT_FOUND;
                    }
                }
            }
        }
    }
    
    // Maintenant, utilisez le switch pour générer la réponse appropriée
    switch (state) {
        case FORMAT_MESSAGE:
            // Construire le message à envoyer
            snprintf(response, response_size, "Message privé de %s: %s", 
                    activeUsers[senderIndex].pseudo, contentStart);
            break;
            
        case MSG_ERROR_NOT_CONNECTED:
            snprintf(response, response_size, "Vous devez être connecté pour envoyer un message privé");
            break;
            
        case ERROR_INVALID_FORMAT:
            snprintf(response, response_size, "Format invalide. Utilisez: @msg <destinataire> <message>");
            break;
            
        case ERROR_MISSING_MESSAGE:
            snprintf(response, response_size, "Message manquant. Utilisez: @msg <destinataire> <message>");
            break;
            
        case ERROR_USER_NOT_FOUND:
            snprintf(response, response_size, "Utilisateur %s non connecté ou inexistant", destPseudo);
            break;
        
        default:
            snprintf(response, response_size, "Erreur inconnue");
            break;
    }
    
    // Par cette alternative utilisant une variable intermédiaire:
    int result;
    if (state == FORMAT_MESSAGE) {
        result = destIndex;
    } else {
        result = -1;
    }
    return result;
}

int helpCmd(const char *payload, const struct sockaddr_in *client, 
           char *response, size_t response_size, User *activeUsers, int numActiveUsers) {
    
    // Ouvrir le fichier README.txt
    FILE *helpFile = fopen("README.txt", "r");
    
    // En cas d'erreur, afficher un message simple
    if (!helpFile) {
        snprintf(response, response_size, "Erreur: impossible d'ouvrir le fichier d'aide.");
    } else {
        // Lire le contenu du fichier
        size_t rd = 0;
        size_t maxSize = response_size - 1; // Garder un octet pour le caractère nul
        
        rd = fread(response, 1, maxSize, helpFile);
        fclose(helpFile);
        
        // S'assurer que la chaîne se termine correctement
        if (rd >= 0 && rd < response_size) {
            response[rd] = '\0';
        } else {
            response[response_size - 1] = '\0';
        }
    }
    
    return 1; // Toujours retourner succès
}

int creditsCmd(const char *payload, const struct sockaddr_in *client, 
           char *response, size_t response_size, User *activeUsers, int numActiveUsers) {
    
    // Ouvrir le fichier Credits.txt
    FILE *creditsFile = fopen("Credits.txt", "r");
    
    // En cas d'erreur, afficher un message simple
    if (!creditsFile) {
        snprintf(response, response_size, "Erreur: impossible d'ouvrir le fichier de crédits.");
    } else {
        // Lire le contenu du fichier
        size_t rd = 0;
        size_t maxSize = response_size - 1; // Garder un octet pour le caractère nul
        
        rd = fread(response, 1, maxSize, creditsFile);
        fclose(creditsFile);
        
        // S'assurer que la chaîne se termine correctement
        if (rd >= 0 && rd < response_size) {
            response[rd] = '\0';
        } else {
            response[response_size - 1] = '\0';
        }
    }
    
    return 1; 
}

int shutdownCmd(const char *payload, const struct sockaddr_in *client, 
           char *response, size_t response_size, User *activeUsers, int numActiveUsers) {
    
    int result = 0;
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client->sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client->sin_port);
    
    int isConnected = 0;
    int isAdmin = 0;
    char userName[PSEUDO_MAX] = "Anonyme";
    int i = 0;
    
    while (i < numActiveUsers && !isConnected) {
        if (strcmp(activeUsers[i].ip, client_ip) == 0 && 
            activeUsers[i].port == client_port && 
            activeUsers[i].isConnected == 1) {
            isConnected = 1;
            isAdmin = activeUsers[i].isAdmin;
            strncpy(userName, activeUsers[i].pseudo, PSEUDO_MAX);
        }
        i++;
    }
    
    if (!isConnected) {
        snprintf(response, response_size, "Vous devez être connecté pour arrêter le serveur.");
    } else if (!isAdmin) {
        snprintf(response, response_size, "Vous n'avez pas les droits administrateur pour arrêter le serveur.");
    } else {
        snprintf(response, response_size, "Arrêt du serveur demandé par %s (admin). Le serveur va s'éteindre...", userName);
        result = 2;
    }
    
    return result;
}