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

int sendPrivateMsg(const char *payload, const struct sockaddr_in *sender_client, 
                  char *response, size_t response_size, User *activeUsers, int numActiveUsers) {
    // Variables pour suivre l'état de la fonction
    int senderIndex = -1;
    int dest_index = -1;
    char destPseudo[PSEUDO_MAX] = {0};
    char *msgStart = NULL;
    char *content_start = NULL;
    int result = -1;
    
    // État de traitement du message
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
    } state = CHECK_SENDER;
    
    // Vérifier que l'expéditeur est connecté
    char sender_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(sender_client->sin_addr), sender_ip, INET_ADDRSTRLEN);
    int sender_port = ntohs(sender_client->sin_port);
    
    // Trouver l'expéditeur
    int i = 0;
    while (i < numActiveUsers && senderIndex == -1) {
        if (strcmp(activeUsers[i].ip, sender_ip) == 0 && 
            activeUsers[i].port == sender_port && 
            activeUsers[i].isConnected == 1) {
            senderIndex = i;
        }
        i++;
    }
    
    // Si l'expéditeur n'est pas trouvé, changer l'état
    if (senderIndex == -1) {
        state = MSG_ERROR_NOT_CONNECTED;
    }
    
    // Machine à états pour traiter le message
    switch (state) {
        case CHECK_SENDER:
            // Expéditeur trouvé, extraire le destinataire
            msgStart = strchr(payload, ' ');
            if (msgStart) {
                msgStart++; // Sauter l'espace
                state = EXTRACT_DESTINATION;
            } else {
                state = ERROR_INVALID_FORMAT;
            }
            /* fall through */
            
        case EXTRACT_DESTINATION:
            if (state == EXTRACT_DESTINATION) {
                if (sscanf(msgStart, "%s", destPseudo) == 1) {
                    state = EXTRACT_MESSAGE;
                } else {
                    state = ERROR_INVALID_FORMAT;
                }
            }
            /* fall through */
            
        case EXTRACT_MESSAGE:
            if (state == EXTRACT_MESSAGE) {
                content_start = strchr(msgStart, ' ');
                if (content_start) {
                    content_start++; // Sauter l'espace
                    state = FIND_DESTINATION;
                } else {
                    state = ERROR_MISSING_MESSAGE;
                }
            }
            /* fall through */
            
        case FIND_DESTINATION:
            if (state == FIND_DESTINATION) {
                i = 0;
                while (i < numActiveUsers && dest_index == -1) {
                    if (strcmp(activeUsers[i].pseudo, destPseudo) == 0 && 
                        activeUsers[i].isConnected == 1) {
                        dest_index = i;
                    }
                    i++;
                }
                
                if (dest_index != -1) {
                    state = FORMAT_MESSAGE;
                } else {
                    state = ERROR_USER_NOT_FOUND;
                }
            }
            
            /* fall through */
        
        case FORMAT_MESSAGE:
            if (state == FORMAT_MESSAGE) {
                // Construire le message à envoyer
                snprintf(response, response_size, "Message privé de %s: %s", activeUsers[senderIndex].pseudo, content_start);
                break;
            }
            /* fall through */
            
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
    
    if (state == FORMAT_MESSAGE) {
        result = dest_index;
    }
    return result;
}

