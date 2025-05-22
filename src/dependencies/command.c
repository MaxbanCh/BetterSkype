#include <string.h>
#include "command.h"
#include <stdio.h>
#include <header.h>
#include "userList.h"
#include <stdlib.h>

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
    // else if (strncmp(payload, "@salon", 6) == 0) {
    //     cmdType = cmdSalon;
    // }
    else if (strncmp(payload, "@create", 7) == 0) {
        cmdType = cmdCreate;
    }
    else if (strncmp(payload, "@list", 5) == 0) {
        cmdType = cmdList;
    }
    else if (strncmp(payload, "@join", 5) == 0) {
        cmdType = cmdJoin;
    }
    else if (strncmp(payload, "@leave", 6) == 0) {
        cmdType = cmdLeave;
    }

    return cmdType;
}

// Modified getUserwithIp function
char *getUserwithIp(User *activeUsers, int numActiveUsers, const struct sockaddr_in *client) {
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client->sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client->sin_port);
    
    // Check if user is connected
    int isConnected = 0;
    char *userName = malloc(PSEUDO_MAX); // Dynamically allocate memory
    if (!userName) {
        perror("Memory allocation failed");
        return NULL;
    }
    
    strcpy(userName, "Anonyme"); // Default value
    int i = 0;
    
    while (i < numActiveUsers && !isConnected) {
        if (strcmp(activeUsers[i].ip, client_ip) == 0 && 
            activeUsers[i].port == client_port && 
            activeUsers[i].isConnected == 1) {
            isConnected = 1;
            strncpy(userName, activeUsers[i].pseudo, PSEUDO_MAX - 1);
            userName[PSEUDO_MAX - 1] = '\0'; // Ensure null termination
        }
        i++;
    }

    return userName; // Now returns dynamically allocated memory that must be freed by the caller
}

int pingCmd(const char *payload, const struct sockaddr_in *client, 
           char *response, size_t responseSize, User *activeUsers, int numActiveUsers) {
    
    // Get client information
    char *userName = getUserwithIp(activeUsers, numActiveUsers, client);
    
    // Format a personalized response
    if (userName != NULL && strcmp(userName, "Anonyme") != 0) {
        snprintf(response, responseSize, "Pong! Serveur en ligne. Vous êtes connecté en tant que %s.", userName);
    } else {
        snprintf(response, responseSize, "Pong! Serveur en ligne. Vous n'êtes pas connecté.");
    }
    
    return 1; // Success - ping response created successfully
}

// Fonction d'enregistrement d'un nouvel utilisateur
int registerUser(const char *payload, const struct sockaddr_in *client, char *response, size_t responseSize, User *activeUsers, int *numActiveUsers) {
    char pseudo[PSEUDO_MAX];
    char password[64];
    memset(pseudo, 0, sizeof(pseudo));
    memset(password, 0, sizeof(password));
    memset(response, 0, responseSize);
    int result = 0; // Variable de retour finale
    
    // Extraire le pseudo et le mot de passe du payload
    // Format: "@register pseudo password"
    if (sscanf(payload, "@register %s %s", pseudo, password) != 2) {
        snprintf(response, responseSize, "Format invalide. Utilisez: @register <pseudo> <password>");
    }
    else {
        // Vérifier si l'utilisateur existe déjà
        int authResult = authenticateClient(pseudo, "");
        if (authResult == 1 || authResult == -1) {
            snprintf(response, responseSize, "Le pseudo %s est déjà utilisé", pseudo);
        }
        else {
            // Ajouter le nouvel utilisateur au fichier
            FILE *file = fopen(USERS_FILE, "a");
            if (!file) {
                file = fopen(USERS_FILE, "w"); // Créer s'il n'existe pas
            }
            
            if (!file) {
                perror("Impossible de créer le fichier utilisateurs");
                snprintf(response, responseSize, "Erreur serveur lors de la création du fichier utilisateurs");
            }
            else {
                // Dans une implémentation réelle, on hasherait le mot de passe ici
                fprintf(file, "%s,%s\n", pseudo, password);
                fclose(file);
                
                // Associer l'utilisateur à son adresse IP et port
                if (associateUser(pseudo, client, activeUsers, numActiveUsers)) {
                    snprintf(response, responseSize, "Utilisateur %s enregistré et connecté avec succès", pseudo);
                    result = 1; // Succès
                }
                else {
                    snprintf(response, responseSize, "Utilisateur %s enregistré mais erreur lors de la connexion", pseudo);
                }
            }
        }
    }
    
    return result;
}

// Traitement de la commande @connect
int connectCmd(const char *payload, const struct sockaddr_in *client, char *response, size_t responseSize, User *activeUsers, int *numActiveUsers, Salon *salon) {
    char pseudo[PSEUDO_MAX];
    char password[64];
    memset(pseudo, 0, sizeof(pseudo));
    memset(password, 0, sizeof(password));
    memset(response, 0, responseSize);
    int result = 0; // Variable de retour finale
    int continueProcessing = 1; // Variable pour contrôler le flux d'exécution

    printf("salon: %s\n", salon->name);
    
    // Extraction du login et mot de passe
    if (sscanf(payload, "@connect %s %s", pseudo, password) != 2) {
        snprintf(response, responseSize, "Format invalide. Utilisez: @connect login mdp");
        continueProcessing = 0;
    }
    
    if (continueProcessing) {
        char clientIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client->sin_addr), clientIp, INET_ADDRSTRLEN);
        int clientPort = ntohs(client->sin_port);
        printf("Tentative de connexion de %s depuis %s:%d\n", pseudo, clientIp, clientPort);
        // Chercher si un utilisateur est déjà connecté depuis cette adresse
        for (int i = 0; i < *numActiveUsers && continueProcessing; i++) {
            if (strcmp(activeUsers[i].ip, clientIp) == 0 && 
                activeUsers[i].port == clientPort && 
                activeUsers[i].isConnected == 1) {
                
                // Si c'est le même utilisateur qui essaie de se reconnecter
                if (strcmp(activeUsers[i].pseudo, pseudo) == 0) {
                    snprintf(response, responseSize, "Vous êtes déjà connecté en tant que %s", pseudo);
                    continueProcessing = 0;
                }
                
                // Sinon, déconnecter l'utilisateur existant
                else {
                    printf("Déconnexion de %s (même client tente une nouvelle connexion)\n", 
                           activeUsers[i].pseudo);
                    activeUsers[i].isConnected = 0;
                }
            }
        }
    }
    
    // Vérifier si l'utilisateur est déjà connecté depuis un autre client
    if (continueProcessing && isUserConnected(pseudo, activeUsers, *numActiveUsers)) {
        snprintf(response, responseSize, "L'utilisateur %s est déjà connecté depuis un autre terminal", pseudo);
        continueProcessing = 0;
    }

    // Authentifier l'utilisateur
    if (continueProcessing) {
        int authResult = authenticateClient(pseudo, password);
        
        if (authResult == 1) {
            // Authentification réussie
            if (associateUser(pseudo, client, activeUsers, numActiveUsers)) {
                snprintf(response, responseSize, "Connecté en tant que %s", pseudo);
                // Ajouter l'utilisateur au salon
                if (joinSalon(salon, pseudo) == 0){
                    printf("L'utilisateur %s a rejoint le salon %s\n", pseudo, salon->name);
                    snprintf(response, responseSize, "Connecté et ajouté au salon %s", salon->name);
                } else {
                    snprintf(response, responseSize, "Connecté mais impossible d'ajouter au salon %s", salon->name);
                }
            
                result = 1;
            } else {
                snprintf(response, responseSize, "Erreur serveur: impossible d'ajouter l'utilisateur à la liste active");
            }
        } else if (authResult == -1) {
            // Mot de passe incorrect
            snprintf(response, responseSize, "Mot de passe invalide pour l'utilisateur %s", pseudo);
        } else {
            // L'utilisateur n'existe pas, l'enregistrer directement
            char registerPayload[BUFFER_MAX];
            snprintf(registerPayload, sizeof(registerPayload), "@register %s %s", pseudo, password);
            
            // Utiliser la nouvelle version de registerUser
            result = registerUser(registerPayload, client, response, responseSize, activeUsers, numActiveUsers);
        }
    }
    
    // Si on arrive ici sans résultat et sans message d'erreur spécifique
    if (continueProcessing && result == 0 && response[0] == '\0') {
        snprintf(response, responseSize, "Erreur inconnue lors de l'authentification");
    }
    
    return result;
}

// Traite la commande @disconnect
int disconnectCmd(const char *payload, const struct sockaddr_in *client, char *response, size_t responseSize, User *activeUsers, int numActiveUsers) {
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
            snprintf(response, responseSize, "Déconnecté: %s", activeUsers[i].pseudo);
            return 1;
        }
    }
    
    snprintf(response, responseSize, "Vous n'êtes pas connecté");
    return 0;
}

int sendPrivateMsg(const char *payload, const struct sockaddr_in *senderClient, 
                  char *response, size_t responseSize, User *activeUsers, int numActiveUsers) {
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
            snprintf(response, responseSize, "Message privé de %s: %s", 
                    activeUsers[senderIndex].pseudo, contentStart);
            break;
            
        case MSG_ERROR_NOT_CONNECTED:
            snprintf(response, responseSize, "Vous devez être connecté pour envoyer un message privé");
            break;
            
        case ERROR_INVALID_FORMAT:
            snprintf(response, responseSize, "Format invalide. Utilisez: @msg <destinataire> <message>");
            break;
            
        case ERROR_MISSING_MESSAGE:
            snprintf(response, responseSize, "Message manquant. Utilisez: @msg <destinataire> <message>");
            break;
            
        case ERROR_USER_NOT_FOUND:
            snprintf(response, responseSize, "Utilisateur %s non connecté ou inexistant", destPseudo);
            break;
        
        default:
            snprintf(response, responseSize, "Erreur inconnue");
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
           char *response, size_t responseSize, User *activeUsers, int numActiveUsers) {
    
    // Ouvrir le fichier README.txt
    FILE *helpFile = fopen("README.txt", "r");
    
    // En cas d'erreur, afficher un message simple
    if (!helpFile) {
        snprintf(response, responseSize, "Erreur: impossible d'ouvrir le fichier d'aide.");
    } else {
        // Lire le contenu du fichier
        size_t rd = 0;
        size_t maxSize = responseSize - 1; // Garder un octet pour le caractère nul
        
        rd = fread(response, 1, maxSize, helpFile);
        fclose(helpFile);
        
        // S'assurer que la chaîne se termine correctement
        if (rd >= 0 && rd < responseSize) {
            response[rd] = '\0';
        } else {
            response[responseSize - 1] = '\0';
        }
    }
    
    return 1; // Toujours retourner succès
}

int creditsCmd(const char *payload, const struct sockaddr_in *client, 
           char *response, size_t responseSize, User *activeUsers, int numActiveUsers) {
    
    // Ouvrir le fichier Credits.txt
    FILE *creditsFile = fopen("Credits.txt", "r");
    
    // En cas d'erreur, afficher un message simple
    if (!creditsFile) {
        snprintf(response, responseSize, "Erreur: impossible d'ouvrir le fichier de crédits.");
    } else {
        // Lire le contenu du fichier
        size_t rd = 0;
        size_t maxSize = responseSize - 1; // Garder un octet pour le caractère nul
        
        rd = fread(response, 1, maxSize, creditsFile);
        fclose(creditsFile);
        
        // S'assurer que la chaîne se termine correctement
        if (rd >= 0 && rd < responseSize) {
            response[rd] = '\0';
        } else {
            response[responseSize - 1] = '\0';
        }
    }
    
    return 1; 
}

int shutdownCmd(const char *payload, const struct sockaddr_in *client, 
           char *response, size_t responseSize, User *activeUsers, int numActiveUsers) {
    
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
        snprintf(response, responseSize, "Vous devez être connecté pour arrêter le serveur.");
    } else if (!isAdmin) {
        snprintf(response, responseSize, "Vous n'avez pas les droits administrateur pour arrêter le serveur.");
    } else {
        snprintf(response, responseSize, "Arrêt du serveur demandé par %s (admin). Le serveur va s'éteindre...", userName);
        result = 2;
    }
    
    return result;
}

// Traite la commande @upload
// Traite la commande @upload
int uploadCmd(const char *payload, const struct sockaddr_in *client, 
             char *response, size_t responseSize, User *activeUsers, int numActiveUsers) {
    
    int result = 0;
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client->sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client->sin_port);
    
    // Vérifier que l'utilisateur est connecté
    int isConnected = 0;
    char userName[PSEUDO_MAX];
    memset(userName, 0, sizeof(userName));

    // Plus tard dans le code, avant d'utiliser userName:
    if (userName[0] == '\0') {
        // Utilisateur non identifié
        snprintf(response, responseSize, "Vous devez être connecté pour cette action.");
    } else {
        // Utilisateur identifié
        snprintf(response, responseSize, "Action effectuée par %s", userName);
    }
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
    
    if (!isConnected) {
        snprintf(response, responseSize, "Vous devez être connecté pour envoyer un fichier.");
        return result;
    }
    
    // Extraire le nom du fichier
    char filename[256];
    char baseName[256]; // Pour stocker seulement le nom du fichier sans chemin
    memset(filename, 0, sizeof(filename));
    memset(baseName, 0, sizeof(baseName));
    
    if (sscanf(payload, "@upload %255s", filename) != 1) {
        snprintf(response, responseSize, "Format invalide. Utilisez: @upload <nom_fichier>");
        return result;
    }
    
    // Extraire le nom de base du fichier (sans chemin)
    const char *basePtr = strrchr(filename, '/');
    if (basePtr) {
        strncpy(baseName, basePtr + 1, sizeof(baseName) - 1);
    } else {
        // Pas de '/' trouvé, vérifier aussi '\'
        basePtr = strrchr(filename, '\\');
        if (basePtr) {
            strncpy(baseName, basePtr + 1, sizeof(baseName) - 1);
        } else {
            strncpy(baseName, filename, sizeof(baseName) - 1);
        }
    }
    
    // Vérifier que le nom de fichier est valide (pas de caractères spéciaux dangereux)
    for (i = 0; i < strlen(baseName); i++) {
        if (baseName[i] < 32 || baseName[i] > 126) {
            snprintf(response, responseSize, "Nom de fichier invalide: contient des caractères non autorisés.");
            return result;
        }
    }
    
    // Construire le chemin complet vers le répertoire de réception
    char destPath[512];
    snprintf(destPath, sizeof(destPath), "files/receive/%s", baseName);
    
    // Préparer la réponse avec le chemin de destination
    snprintf(response, responseSize, "FILES:%s|Le serveur a recu le fichier %s envoyé par %s", 
         destPath, baseName, userName);
    
    // Retourner un code spécial pour indiquer que le transfert de fichier doit être traité
    result = 3;  // Code spécial pour le transfert de fichier
    
    return result;
}


// Traite la commande @download
int downloadCmd(const char *payload, const struct sockaddr_in *client, 
               char *response, size_t responseSize, User *activeUsers, int numActiveUsers) {
    
    int result = 0;
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client->sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client->sin_port);
    
    // Vérifier que l'utilisateur est connecté
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
    
    if (!isConnected) {
        snprintf(response, responseSize, "Vous devez être connecté pour télécharger un fichier.");
        return result;
    }
    
    // Extraire le nom du fichier
    char filename[256];
    char baseName[256]; // Pour stocker seulement le nom du fichier sans chemin
    memset(filename, 0, sizeof(filename));
    memset(baseName, 0, sizeof(baseName));
    
    if (sscanf(payload, "@download %255s", filename) != 1) {
        snprintf(response, responseSize, "Format invalide. Utilisez: @download <nom_fichier>");
        return result;
    }
    
    // Extraire le nom de base du fichier (sans chemin)
    const char *basePtr = strrchr(filename, '/');
    if (basePtr) {
        strncpy(baseName, basePtr + 1, sizeof(baseName) - 1);
    } else {
        // Pas de '/' trouvé, vérifier aussi '\'
        basePtr = strrchr(filename, '\\');
        if (basePtr) {
            strncpy(baseName, basePtr + 1, sizeof(baseName) - 1);
        } else {
            strncpy(baseName, filename, sizeof(baseName) - 1);
        }
    }
    
    // Construire le chemin complet pour vérifier si le fichier existe
    char sourcePath[512];
    snprintf(sourcePath, sizeof(sourcePath), "files/send/%s", baseName);
    
    // Vérifier que le fichier existe dans le dossier files/send/
    FILE *file = fopen(sourcePath, "r");
    if (!file) {
        snprintf(response, responseSize, "Le fichier %s n'existe pas dans le dossier de partage.", baseName);
        return result;
    }
    fclose(file);
    
    // Préparer la réponse avec le chemin source
    snprintf(response, responseSize, "FILES:%s|Le serveur a envoyé le fichier %s demandé par %s", 
         sourcePath, baseName, userName);
    
    // Retourner un code spécial pour indiquer que le transfert de fichier doit être traité
    result = 4;  // Code spécial pour le transfert de fichier download
    
    return result;
}

int createSalonCmd(const char *payload, const struct sockaddr_in *client, 
           char *response, size_t response_size, salonList *salons, User *activeUsers, int numActiveUsers) {
    
    // Format attendu: "@create salon"
    char salonName[SALON_NAME_MAX];
    if (sscanf(payload, "@create %s", salonName) != 1) {
        snprintf(response, response_size, "Format invalide. Utilisez: @create <salon>");
        return 0;
    }
    
    // Vérifier si le salon existe déjà
    if (findSalon(salons, salonName)) {
        snprintf(response, response_size, "Le salon %s existe déjà.", salonName);
        return 0;
    }

    char *username = getUserwithIp(activeUsers, numActiveUsers, client);
    if (username == NULL) {
        snprintf(response, response_size, "Vous devez être connecté pour créer un salon.");
        return 0;
    }
    // Créer le salon
    Salon *newSalon = createSalon(salonName, username);
    if (!newSalon) {
        snprintf(response, response_size, "Erreur lors de la création du salon %s.", salonName);
        return 0;
    }

    // Ajouter le salon à la liste
    addSalon(salons, newSalon);
    
    snprintf(response, response_size, "Salon %s créé avec succès.", salonName);
    return 1;
}

int joinCmd(const char *payload, const struct sockaddr_in *client, 
           char *response, size_t response_size, salonList *salons, User *activeUsers, int numActiveUsers) {
    
    // Format attendu: "@join salon"
    char salonName[SALON_NAME_MAX];
    if (sscanf(payload, "@join %s", salonName) != 1) {
        snprintf(response, response_size, "Format invalide. Utilisez: @join <salon>");
        return 0;
    }
    
    // Vérifier si le salon existe
    Salon *salon = findSalon(salons, salonName);
    if (!salon) {
        snprintf(response, response_size, "Le salon %s n'existe pas.", salonName);
        return 0;
    }

    // Get username from client address
    char *username = getUserwithIp(activeUsers, numActiveUsers, client);
    if (username == NULL) {
        snprintf(response, response_size, "Vous devez être connecté pour rejoindre un salon.");
        return 0;
    }

    // Verifier si l'utilisateur n'est pas deja dans un autre salon
    salonNode *salonN = salons->head;
    while (salonN != NULL) {
        if (isUserInList(salonN->salon->users, username)) {
            snprintf(response, response_size, "Vous êtes déjà dans le salon %s.", salonN->salon->name);
            return 0;
        }
        salonN = salonN->next;
    }
    
    // Join the salon with the username
    int result = joinSalon(salon, username);
    if (result == 0) {
        snprintf(response, response_size, "Vous avez rejoint le salon %s.", salonName);
    } else {
        snprintf(response, response_size, "Erreur lors de la connexion au salon %s.", salonName);
    }
    return result;
}

int sendMessageSalon(const char *payload, const struct sockaddr_in *client, 
                   char *response, size_t response_size, salonList *salons, User *activeUsers, int numActiveUsers,
                   userList **users) {
    
    // Format attendu: "@msg message"

    int findUser = 0;
    salonNode *salonN = salons->head;
    
    // Recherche du salon de l'utilisateur
    char *username = getUserwithIp(activeUsers, numActiveUsers, client);
    if (username == NULL) {
        snprintf(response, response_size, "Vous devez être connecté pour envoyer un message.");
        return 0;
    }
    
    while (findUser == 0 && salonN != NULL) {
        if (isUserInList(salonN->salon->users, username)) {
            findUser = 1;
        }
        else {
            salonN = salonN->next;
        }
    }

    if (findUser == 0) {
        snprintf(response, response_size, "Vous devez être dans un salon pour envoyer un message.");
        return 0;
    }

    Salon *salon = salonN->salon;
    

    // Formater le message: "expediteur : message"
    snprintf(response, response_size, "%s : %s", username, payload);
    
    // Copier la référence aux utilisateurs du salon pour que le serveur puisse l'utiliser
    if (users != NULL) {
        *users = salon->users;
    }
    
    // Enregistrer le message dans l'historique du salon (optionnel)
    saveMessage(salon, payload, username);
    
    return 1; // Succès avec la liste des utilisateurs
}

int leaveCmd(const char *payload, const struct sockaddr_in *client, 
           char *response, size_t response_size, salonList *salons, User *activeUsers, int numActiveUsers) {
    
    // Format attendu: "@leave salon"
    char salonName[SALON_NAME_MAX];
    if (sscanf(payload, "@leave %s", salonName) != 1) {
        snprintf(response, response_size, "Format invalide. Utilisez: @leave <salon>");
        return 0;
    }
    
    // Vérifier si le salon existe
    Salon *salon = findSalon(salons, salonName);
    if (!salon) {
        snprintf(response, response_size, "Le salon %s n'existe pas.", salonName);
        return 0;
    }

    // Get username from client address
    char *username = getUserwithIp(activeUsers, numActiveUsers, client);
    if (username == NULL) {
        snprintf(response, response_size, "Vous devez être connecté pour quitter un salon.");
        return 0;
    }
    
    // Quitter le salon
    int result = leaveSalon(salon, username);
    if (result == 0) {
        snprintf(response, response_size, "Vous avez quitté le salon %s.", salonName);
    } else {
        snprintf(response, response_size, "Erreur lors de la sortie du salon %s.", salonName);
    }
    
    return result;
}