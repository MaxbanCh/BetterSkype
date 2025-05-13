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