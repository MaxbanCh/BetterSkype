#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "message.h"  // structure contenant les champs du message
#include "serveur.h"      
#include <signal.h>
#include "header.h"  
#include "dependencies/TCPFile.h"       


// Handler de signal SIGINT (Ctrl+C) pour arrêter proprement le serveur
static void handleSigint(int sig) {
    (void)sig;  
    printf("\nArrêt serveur\n");
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

// Fonction qui parse le buffer en 5 parties : ip, pseudo, num, total, payload
int parseMessage(const char *buffer, MessageInfo *msg) {
    const char *p = buffer, *sep;
    size_t len;
    char tmp[16];

    // 1) Extraction de l'adresse IP de l'expéditeur
    sep = strstr(p, FIELD_DELIM);
    if (!sep) return -1;
    len = sep - p;
    memcpy(msg->sender_ip, p, len);
    msg->sender_ip[len] = '\0';

    // 2) Extraction du pseudo du destinataire
    p = sep + strlen(FIELD_DELIM);
    sep = strstr(p, FIELD_DELIM);
    if (!sep) return -1;
    len = sep - p;
    memcpy(msg->dest_pseudo, p, len);
    msg->dest_pseudo[len] = '\0';

    // 3) Extraction du numéro de la partie du message
    p = sep + strlen(FIELD_DELIM);
    sep = strstr(p, FIELD_DELIM);
    if (!sep) return -1;
    len = sep - p;
    memcpy(tmp, p, len); tmp[len] = '\0';
    msg->part_num = atoi(tmp); // conversion en entier

    // 4) Extraction du nombre total de parties
    p = sep + strlen(FIELD_DELIM);
    sep = strstr(p, PAYLOAD_DELIM);     
    if (!sep) return -1;
    len = sep - p;
    memcpy(tmp, p, len); tmp[len] = '\0';
    msg->total_parts = atoi(tmp);

    // 5) Extraction du contenu (payload)
    p = sep + strlen(PAYLOAD_DELIM);
    strncpy(msg->payload, p, sizeof(msg->payload)-1);
    msg->payload[sizeof(msg->payload)-1] = '\0';

    return 0; 
}


void printMessage(const MessageInfo *m) {
    printf("Expéditeur IP       : %s\n", m->sender_ip);
    printf("Destinataire pseudo : %s\n", m->dest_pseudo);
    printf("Partie              : %d / %d\n", m->part_num, m->total_parts);
    printf("Contenu             : %s\n\n", m->payload);
}

// Fonction qui envoie un accusé de réception à l’expéditeur du message
void sendAcR(int sockfd, const struct sockaddr_in *cli) {
    const char *acR = "OK"; 
    socklen_t len = sizeof(*cli);
    if (sendto(sockfd, acR, strlen(acR), 0, (const struct sockaddr*)cli, len) < 0) {
        perror("sendto"); 
    }
}

void handleFileTransfer(const MessageInfo *msg, int sockfd, const struct sockaddr_in *client) {
    // Format attendu: "@upload:<fichier>" ou "@download:<fichier>"
    printf("Traitement du transfert de fichier...\n");
    
    char command[BUFFER_MAX];
    strncpy(command, msg->payload, sizeof(command) - 1);
    
    char *operation, *filename;
    
    // Extraire les informations du message
    operation = strtok(command, " ");
    if (!operation) {
        printf("Format de commande invalide\n");
        return;
    }
    
    filename = strtok(NULL, " ");
    if (!filename) {
        printf("Nom de fichier manquant\n");
        return;
    }
    
    printf("Initialisation transfert de fichier avec %s...\n", inet_ntoa(client->sin_addr));
    
    // Déterminer l'opération complémentaire à envoyer au client
    char clientOperation[10];
    if (strcmp(operation, "@upload") == 0) {
        // Le client veut uploader, le serveur va recevoir
        strcpy(clientOperation, "UPLOAD");
    } else if (strcmp(operation, "@download") == 0) {
        // Le client veut télécharger, le serveur va envoyer
        strcpy(clientOperation, "DOWNLOAD");
    } else {
        printf("Opération non reconnue: %s\n", operation);
        return;
    }
    
    // Envoi d'une commande TCP au client pour l'informer qu'une connexion TCP va être établie
    char tcpResponse[BUFFER_MAX];
    snprintf(tcpResponse, sizeof(tcpResponse), "TCP:%s:%s:%s", clientOperation, 
             filename, inet_ntoa(client->sin_addr));
    
    socklen_t len = sizeof(*client);
    if (sendto(sockfd, tcpResponse, strlen(tcpResponse), 0, (const struct sockaddr*)client, len) < 0) {
        perror("Erreur envoi commande TCP");
        return;
    }
    
    printf("Commande TCP envoyée: %s\n", tcpResponse);
    
    // Initialiser le mode serveur TCP et attendre une connexion
    printf("Démarrage du serveur TCP...\n");
    int socketTCP = initTCPSocketServer();
    if (socketTCP < 0) {
        printf("Échec initialisation serveur TCP\n");
        return;
    }
    
    printf("En attente d'une connexion TCP...\n");
    int clientTCP = connexionTCP(socketTCP);
    if (clientTCP < 0)  {
        printf("Échec connexion TCP avec le client\n");
        closeServer(socketTCP, clientTCP); // Pass the appropriate second argument based on the function definition
        return;
    }
    
    int result;
    if (strcmp(operation, "@upload") == 0) {
        // Client upload = serveur reçoit
        printf("Réception du fichier %s en cours...\n", filename);
        result = receiveFile(clientTCP, filename);
        if (result == 0) {
            printf("Fichier reçu avec succès\n");
        } else {
            printf("Erreur lors de la réception du fichier (code %d)\n", result);
        }
    } else if (strcmp(operation, "@download") == 0) {
        // Client download = serveur envoie
        printf("Envoi du fichier %s en cours...\n", filename);
        result = sendFile(clientTCP, filename);
        if (result == 0) {
            printf("Fichier envoyé avec succès\n");
        } else {
            printf("Erreur lors de l'envoi du fichier (code %d)\n", result);
        }
    }
    
    // Fermer les connexions TCP
    closeServer(socketTCP, clientTCP);
}

int main() {
    
    signal(SIGINT, handleSigint);

    // Initialisation de la socket UDP
    int sockfd = initSocket();

    char buffer[BUFFER_MAX];          // buffer de réception de taille BUFFER_MAX
    struct sockaddr_in client;        // structure pour l'adresse du client

    // Boucle infinie : réception, parsing, affichage et envoi d'un Accusé de Réception
    while (1) {
        ssize_t r = recvMessage(sockfd, buffer, sizeof(buffer), &client);
        if (r < 0) {
            // en cas d’erreur de réception, affiche l’erreur et continue
            perror("recvMessage");
            continue;
        }

        // parsing du message reçu dans une structure MessageInfo
        MessageInfo msg;
        if (parseMessage(buffer, &msg) == 0) {
            // affichage du message s’il est bien parsé
            printMessage(&msg);
        } else {
            
            fprintf(stderr, "[WARN] parsing failed: %s\n", buffer);
        }

        if (strncmp(msg.payload, "@upload", 7) == 0 || strncmp(msg.payload, "@download", 9) == 0) {
            // Si le message commence par @upload ou @download, traiter le transfert de fichier
            handleFileTransfer(&msg, sockfd, &client);
        } else {
            // Sinon, traiter le message comme un message normal
            printf("Message reçu : %s\n", buffer);
        }

        // envoi d’un accusé de réception au client qui a envoyé le message
        sendAcR(sockfd, &client);
    }

    
    return EXIT_SUCCESS;
}