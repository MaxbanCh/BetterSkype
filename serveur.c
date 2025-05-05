#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "MessageInfo.h"  
#include "serveur.h" 
#include <signal.h>

#define PORT         12345
#define BUFFER_MAX   1024
#define FIELD_DELIM   "//"
#define PAYLOAD_DELIM "/#/#00"

static void handleSigint(int sig) {
    (void)sig;
    printf("\nArrêt serveur\n");
    exit(EXIT_SUCCESS);
}

// crée et bind la socket UDP
int initSocket(void) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { perror("socket"); exit(EXIT_FAILURE); }

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); 
        close(fd); 
        exit(EXIT_FAILURE);
    }
    printf("Serveur UDP démarré sur le port %d\n", PORT);
    return fd;
}


ssize_t recvMessage(int sockfd, char *buffer, size_t buflen, struct sockaddr_in *cli) {
    socklen_t len = sizeof(*cli);
    ssize_t r = recvfrom(sockfd, buffer, buflen - 1, 0,(struct sockaddr*)cli, &len);
    if (r < 0) {
        perror("recvfrom");
        return -1;
    }
    buffer[r] = '\0';
    return r;
}

int parseMessage(const char *buffer, MessageInfo *msg) { // buffer : "ipExp//destPseudo//numPart//total_parts/#/#00payload"
    const char *p = buffer, *sep;
    size_t len;
    char tmp[16];

    // 1) ip de l'expéditeur
    sep = strstr(p, FIELD_DELIM);
    if (!sep) return -1;
    len = sep - p;
    memcpy(msg->sender_ip, p, len);
    msg->sender_ip[len] = '\0';

    // 2) pseudo du destinataire 
    p = sep + strlen(FIELD_DELIM);
    sep = strstr(p, FIELD_DELIM);
    if (!sep) return -1;
    len = sep - p;
    memcpy(msg->dest_pseudo, p, len);
    msg->dest_pseudo[len] = '\0';

    // 3) numéro de la partie du message
    p = sep + strlen(FIELD_DELIM);
    sep = strstr(p, FIELD_DELIM);
    if (!sep) return -1;
    len = sep - p;
    memcpy(tmp, p, len); tmp[len] = '\0';
    msg->part_num = atoi(tmp);

    // 4) nombre total de parties
    p = sep + strlen(FIELD_DELIM);
    sep = strstr(p, PAYLOAD_DELIM);     
    if (!sep) return -1;
    len = sep - p;
    memcpy(tmp, p, len); tmp[len] = '\0';
    msg->total_parts = atoi(tmp);

    // payload
    p = sep + strlen(PAYLOAD_DELIM);
    strncpy(msg->payload, p, sizeof(msg->payload)-1);
    msg->payload[sizeof(msg->payload)-1] = '\0';

    return 0;
}

// affichage
void printMessage(const MessageInfo *m) {
    printf("Expéditeur IP       : %s\n", m->sender_ip);
    printf("Destinataire pseudo : %s\n", m->dest_pseudo);
    printf("Partie              : %d / %d\n", m->part_num, m->total_parts);
    printf("Contenu             : %s\n\n", m->payload);
}

// accusé de réception
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

    char buffer[BUFFER_MAX];
    struct sockaddr_in client;

    // Boucle infinie de réception / traitement / ACK
    while (1) {
        ssize_t r = recvMessage(sockfd, buffer, sizeof(buffer), &client);
        if (r < 0) {
            // en cas d’erreur, on continue à la prochaine itération
            perror("recvMessage");
            continue;
        }

        MessageInfo msg;
        if (parseMessage(buffer, &msg) == 0) {
            printMessage(&msg);
        } else {
            fprintf(stderr, "[WARN] parsing failed: %s\n", buffer);
        }

        // Envoi d’un accusé de réception (ACK) à l’expéditeur
        sendAcR(sockfd, &client);
    }

    // jamais atteint
    return EXIT_SUCCESS;
}