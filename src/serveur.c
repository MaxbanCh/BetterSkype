#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "message.h"  // structure contenant les champs du message
#include "serveur.h"      
#include <signal.h>
#include "header.h"       
#include "user.h"


// Tableau global des utilisateurs actifs
User activeUsers[MAX_USERS];
int numActiveUsers = 0;
int serverRunning = 1;

// Handler de signal SIGINT (Ctrl+C) pour arrêter proprement le serveur
static void handleSigint(int sig) {
    (void)sig;  
    printf("\nArrêt serveur\n");
    serverRunning = 0;
    //close(sockfd); 
    exit(EXIT_SUCCESS);
}

// Fonction qui initialise la socket UDP et effectue le bind
int initSocket(void) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0); 
    if (fd < 0) { perror("socket"); exit(EXIT_FAILURE); }

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,         // famille d'adresses : IPv4
        .sin_port        = htons(PORT),     // port d'écoute
        .sin_addr.s_addr = INADDR_ANY       // écoute sur toutes les interfaces
    };

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); 
        close(fd); 
        exit(EXIT_FAILURE);
    }
    printf("Serveur UDP démarré sur le port %d\n", PORT);
    return fd; 
}

// Fonction qui reçoit un message depuis un client UDP
ssize_t recvMessage(int sockfd, char *buffer, size_t buflen, struct sockaddr_in *cli) {
    socklen_t len = sizeof(*cli);  // longueur de l'adresse client
    ssize_t r = recvfrom(sockfd, buffer, buflen - 1, 0, (struct sockaddr*)cli, &len); // réception
    if (r < 0) {
        perror("recvfrom");
        return -1;
    }
    buffer[r] = '\0'; 
    return r;         
}

void printMessage(const MessageInfo *m) {
    printf("Expéditeur IP       : %s\n", m->sender_ip);
    printf("Destinataire pseudo : %s\n", m->dest_pseudo);
    printf("Partie              : %d / %d\n", m->part_num, m->total_parts);
    printf("Contenu             : %s\n\n", m->payload);
}

// Fonction qui envoie un accusé de réception à l’expéditeur du message
void sendAcR(int sockfd, const struct sockaddr_in *cli) {
    const char *acR = "reçu accusé de réception\n"; 
    socklen_t len = sizeof(*cli);
    if (sendto(sockfd, acR, strlen(acR), 0, (const struct sockaddr*)cli, len) < 0) {
        perror("sendto"); 
    }
}

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
int isUserConnected(const char *pseudo) {
    for (int i = 0; i < numActiveUsers; i++) {
        if (strcmp(activeUsers[i].pseudo, pseudo) == 0 && activeUsers[i].isConnected) {
            return 1; // Utilisateur connecté
        }
    }
    return 0; // Utilisateur non connecté
}

// Associe un utilisateur à une adresse IP et port
int associateUser(const char *pseudo, const struct sockaddr_in *client_addr) {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr->sin_addr), ip, INET_ADDRSTRLEN);
    int port = ntohs(client_addr->sin_port);
    
    // Vérifier si l'utilisateur est déjà dans la liste
    for (int i = 0; i < numActiveUsers; i++) {
        if (strcmp(activeUsers[i].pseudo, pseudo) == 0) {
            // Mettre à jour l'utilisateur existant
            strcpy(activeUsers[i].ip, ip);
            activeUsers[i].port = port;
            activeUsers[i].isConnected = 1;
            return 1;
        }
    }
    
    // Ajouter un nouvel utilisateur actif
    if (numActiveUsers >= MAX_USERS) {
        return 0; // Liste pleine
    }
    
    strcpy(activeUsers[numActiveUsers].pseudo, pseudo);
    strcpy(activeUsers[numActiveUsers].ip, ip);
    activeUsers[numActiveUsers].port = port;
    activeUsers[numActiveUsers].isConnected = 1;
    numActiveUsers++;
    return 1;
}

// Traitement de la commande @connect
int connectCmd(const char *payload, const struct sockaddr_in *client, char *response, size_t response_size) {
    char pseudo[PSEUDO_MAX];
    char password[64];
    
    // Extraction du login et mot de passe
    // Format: "@connect login mdp"
    if (sscanf(payload, "@connect %s %s", pseudo, password) != 2) {
        snprintf(response, response_size, "Format invalide. Utilisez: @connect login mdp");
        return 0;
    }
    
    // Vérifier si l'utilisateur est déjà connecté
    if (isUserConnected(pseudo)) {
        snprintf(response, response_size, "L'utilisateur %s est déjà connecté", pseudo);
        return 0;
    }
    
    // Authentifier l'utilisateur
    int auth_result = authenticateClient(pseudo, password);
    
    if (auth_result == 1) {
        // Authentification réussie
        if (associateUser(pseudo, client)) {
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
    } else {
        // L'utilisateur n'existe pas, essayer de l'enregistrer
        if (registerUser(pseudo, password)) {
            // Enregistrement réussi
            if (associateUser(pseudo, client)) {
                snprintf(response, response_size, "Enregistré et connecté en tant que %s", pseudo);
                return 1;
            } else {
                snprintf(response, response_size, "Erreur serveur: impossible d'ajouter l'utilisateur à la liste active");
                return 0;
            }
        } else {
            snprintf(response, response_size, "Échec de l'enregistrement de l'utilisateur %s", pseudo);
            return 0;
        }
    }
}

// Traite la commande @disconnect
int disconnectCmd(const char *payload, const struct sockaddr_in *client, char *response, size_t response_size) {
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


int main() {
    
    signal(SIGINT, handleSigint);

    // Initialisation de la socket UDP
    int sockfd = initSocket();

    char buffer[BUFFER_MAX];          // buffer de réception de taille BUFFER_MAX
    struct sockaddr_in client;        // structure pour l'adresse du client

    // Boucle infinie : réception, parsing, affichage et envoi d'un Accusé de Réception
    while (serverRunning) {
        ssize_t r = recvMessage(sockfd, buffer, sizeof(buffer), &client);
        if (r < 0) {
            perror("recvMessage");
            continue; 
        }

        // Analyser le message reçu
        MessageInfo msg;
        if (parseMessage(buffer, &msg) == 0) {
            printMessage(&msg);
            
            // Vérifier si c'est une commande d'authentification
            if (strncmp(msg.payload, "@connect", 8) == 0) {
                char response[BUFFER_MAX];
                
                // Traiter la commande de connexion
                if (connectCmd(msg.payload, &client, response, sizeof(response))) {
                    // Authentification réussie
                    sendto(sockfd, response, strlen(response), 0, 
                          (struct sockaddr*)&client, sizeof(client));
                    printf("Utilisateur authentifié: %s\n", response);
                } else {
                    // Authentification échouée
                    sendto(sockfd, response, strlen(response), 0, 
                          (struct sockaddr*)&client, sizeof(client));
                }
            } 
            else if (strncmp(msg.payload, "@disconnect", 11) == 0) {
                char response[BUFFER_MAX];
                
                // Traiter la commande de déconnexion
                if (disconnectCmd(msg.payload, &client, response, sizeof(response))) {
                    // Déconnexion réussie
                    sendto(sockfd, response, strlen(response), 0, 
                          (struct sockaddr*)&client, sizeof(client));
                    printf("Utilisateur déconnecté: %s\n", response);
                } else {
                    // Déconnexion échouée
                    sendto(sockfd, response, strlen(response), 0, 
                          (struct sockaddr*)&client, sizeof(client));
                }

            }
            else{
                // Traitement des messages normaux
                // Vérifier si le client est authentifié avant de traiter le message
                int authenticated = 0;
                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(client.sin_addr), client_ip, INET_ADDRSTRLEN);
                
                int i = 0;
                while (i < numActiveUsers && !authenticated) {
                    if (strcmp(activeUsers[i].ip, client_ip) == 0 && 
                        activeUsers[i].isConnected) {
                        authenticated = 1;
                    }
                    i++;
                }
                
                if (authenticated) {
                    // Client authentifié, traitement normal
                    sendAcR(sockfd, &client);
                } else {
                    // Client non authentifié
                    const char *auth_req = "Veuillez vous authentifier avec '@connect login mdp'";
                    sendto(sockfd, auth_req, strlen(auth_req), 0, 
                          (struct sockaddr*)&client, sizeof(client));
                }
            }
        } else {
            fprintf(stderr, "[WARN] échec d'analyse: %s\n", buffer);
            // Envoyer quand même un accusé, même en cas d'erreur d'analyse
            sendAcR(sockfd, &client);
        }
    }
    
    return EXIT_SUCCESS;
}