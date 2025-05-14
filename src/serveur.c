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

int sendPrivateMsg(const char *payload, const struct sockaddr_in *sender_client, 
                  char *response, size_t response_size, User *activeUsers, int numActiveUsers) {
    // Variables pour suivre l'état de la fonction
    int sender_index = -1;
    int dest_index = -1;
    char dest_pseudo[PSEUDO_MAX] = {0};
    char *msg_start = NULL;
    char *content_start = NULL;
    int result = -1;
    
    // État de traitement du message
    enum {
        CHECK_SENDER,
        EXTRACT_DESTINATION,
        EXTRACT_MESSAGE,
        FIND_DESTINATION,
        FORMAT_MESSAGE,
        ERROR_NOT_CONNECTED,
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
    while (i < numActiveUsers && sender_index == -1) {
        if (strcmp(activeUsers[i].ip, sender_ip) == 0 && 
            activeUsers[i].port == sender_port && 
            activeUsers[i].isConnected == 1) {
            sender_index = i;
        }
        i++;
    }
    
    // Si l'expéditeur n'est pas trouvé, changer l'état
    if (sender_index == -1) {
        state = ERROR_NOT_CONNECTED;
    }
    
    // Machine à états pour traiter le message
    switch (state) {
        case CHECK_SENDER:
            // Expéditeur trouvé, extraire le destinataire
            msg_start = strchr(payload, ' ');
            if (msg_start) {
                msg_start++; // Sauter l'espace
                state = EXTRACT_DESTINATION;
            } else {
                state = ERROR_INVALID_FORMAT;
            }
            /* fall through */
            
        case EXTRACT_DESTINATION:
            if (state == EXTRACT_DESTINATION) {
                if (sscanf(msg_start, "%s", dest_pseudo) == 1) {
                    state = EXTRACT_MESSAGE;
                } else {
                    state = ERROR_INVALID_FORMAT;
                }
            }
            /* fall through */
            
        case EXTRACT_MESSAGE:
            if (state == EXTRACT_MESSAGE) {
                content_start = strchr(msg_start, ' ');
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
                    if (strcmp(activeUsers[i].pseudo, dest_pseudo) == 0 && 
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
            
            
        case FORMAT_MESSAGE:
            if (state == FORMAT_MESSAGE) {
                // Construire le message à envoyer
                snprintf(response, response_size, "Message privé de %s: %s", activeUsers[sender_index].pseudo, content_start);
            }
            break;
            
        case ERROR_NOT_CONNECTED:
            snprintf(response, response_size, "Vous devez être connecté pour envoyer un message privé");
            break;
            
        case ERROR_INVALID_FORMAT:
            snprintf(response, response_size, "Format invalide. Utilisez: @msg <destinataire> <message>");
            break;
            
        case ERROR_MISSING_MESSAGE:
            snprintf(response, response_size, "Message manquant. Utilisez: @msg <destinataire> <message>");
            break;
            
        case ERROR_USER_NOT_FOUND:
            snprintf(response, response_size, "Utilisateur %s non connecté ou inexistant", dest_pseudo);
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

int main() {
    
    // Tableau global des utilisateurs actifs
    User activeUsers[MAX_USERS];
    int numActiveUsers = 0;

    signal(SIGINT, handleSigint); // Gestion du signal SIGINT (Ctrl+C)

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
                if (connectCmd(msg.payload, &client, response, sizeof(response), activeUsers, &numActiveUsers)) {
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
                if (disconnectCmd(msg.payload, &client, response, sizeof(response), activeUsers, numActiveUsers)) {
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
            else if (strncmp(msg.payload, "@msg", 4) == 0) {
                char response[BUFFER_MAX];
                
                // Traiter la commande @msg
                int dest_index = sendPrivateMsg(msg.payload, &client, response, sizeof(response), activeUsers, numActiveUsers);
                
                if (dest_index >= 0) {
                    // Configurer l'adresse du destinataire
                    struct sockaddr_in dest_addr;
                    dest_addr.sin_family = AF_INET;
                    inet_pton(AF_INET, activeUsers[dest_index].ip, &dest_addr.sin_addr);
                    dest_addr.sin_port = htons(activeUsers[dest_index].port);
                    
                    // Envoyer le message au destinataire
                    sendto(sockfd, response, strlen(response), 0, 
                        (struct sockaddr*)&dest_addr, sizeof(dest_addr));
                    
                    // Envoyer confirmation à l'expéditeur
                    char confirm[BUFFER_MAX];
                    snprintf(confirm, sizeof(confirm), "Message envoyé à %s", activeUsers[dest_index].pseudo);
                    sendto(sockfd, confirm, strlen(confirm), 0, 
                        (struct sockaddr*)&client, sizeof(client));
                    
                    printf("Message privé envoyé à %s\n", activeUsers[dest_index].pseudo);
                } else {
                    // Échec de l'envoi (utilisateur non connecté ou message mal formaté)
                    sendto(sockfd, response, strlen(response), 0, 
                        (struct sockaddr*)&client, sizeof(client));
                }
            } 
            else if (strncmp(msg.payload, "@register", 9) == 0) {
                char response[BUFFER_MAX];
                
                // Traiter la commande d'enregistrement
                if (registerUser(msg.payload, &client, response, sizeof(response), activeUsers, &numActiveUsers)) {
                    // Enregistrement réussi
                    sendto(sockfd, response, strlen(response), 0, 
                          (struct sockaddr*)&client, sizeof(client));
                    printf("Utilisateur enregistré: %s\n", response);
                } else {
                    // Enregistrement échoué
                    sendto(sockfd, response, strlen(response), 0, 
                          (struct sockaddr*)&client, sizeof(client));
                }
}
            else{
                // Traitement des messages normaux                // Vérifier si le client est authentifié avant de traiter le message
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