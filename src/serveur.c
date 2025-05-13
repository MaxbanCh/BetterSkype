#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "message.h"  // structure contenant les champs du message
#include "serveur.h"      
#include <signal.h>
#include "header.h"       


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
    const char *acR = "reçu accusé de réception\n"; 
    socklen_t len = sizeof(*cli);
    if (sendto(sockfd, acR, strlen(acR), 0, (const struct sockaddr*)cli, len) < 0) {
        perror("sendto"); 
    }
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

        // envoi d’un accusé de réception au client qui a envoyé le message
        sendAcR(sockfd, &client);
    }

    
    return EXIT_SUCCESS;
}