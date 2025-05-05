#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "MessageInfo.h"   
#define PORT 12345
#define DELIM      "//"

// -------- Prototypes --------
int   init_server_socket(void);
ssize_t recv_message(int sockfd, char *buffer, struct sockaddr_in *cli);
void  parse_message(const char *buffer, MessageInfo *msg);
void  print_message(const MessageInfo *msg);
void  send_ack(int sockfd, const struct sockaddr_in *cli);

// -------- Implémentations --------

// 1) Crée et bind une socket UDP sur PORT
int init_server_socket(void) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { perror("socket"); exit(EXIT_FAILURE); }

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_port        = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(fd); exit(EXIT_FAILURE);
    }

    printf("[INFO] Serveur UDP démarré sur le port %d\n", PORT);
    return fd;
}

// 2) Reçoit un datagramme UDP et retourne la longueur reçue
ssize_t recv_message(int sockfd, char *buffer, struct sockaddr_in *cli) {
    socklen_t len = sizeof(*cli);
    ssize_t r = recvfrom(sockfd, buffer, BUFFER_MAX - 1, 0,
                         (struct sockaddr*)cli, &len);
    if (r < 0) {
        perror("recvfrom");
        return -1;
    }
    buffer[r] = '\0';
    return r;
}

// 3) Parse buffer "ip//pseudo//part_num//total_parts//payload" dans messageInfo
void parse_message(const char *buffer, MessageInfo *msg) {
    const char *p = buffer, *sep;
    size_t len;
    char tmp[16];

    // Champ 1 : sender_ip
    sep = strstr(p, DELIM);
    len = sep - p;
    memcpy(msg->sender_ip, p, len);
    msg->sender_ip[len] = '\0';

    // Champ 2 : dest_pseudo
    p = sep + strlen(DELIM);
    sep = strstr(p, DELIM);
    len = sep - p;
    memcpy(msg->dest_pseudo, p, len);
    msg->dest_pseudo[len] = '\0';

    // Champ 3 : part_num
    p = sep + strlen(DELIM);
    sep = strstr(p, DELIM);
    len = sep - p;
    memcpy(tmp, p, len);
    tmp[len] = '\0';
    msg->part_num = atoi(tmp);

    // Champ 4 : total_parts
    p = sep + strlen(DELIM);
    sep = strstr(p, DELIM);
    len = sep - p;
    memcpy(tmp, p, len);
    tmp[len] = '\0';
    msg->total_parts = atoi(tmp);

    // Payload
    p = sep + strlen(DELIM);
    strncpy(msg->payload, p, BUFFER_MAX - 1);
    msg->payload[BUFFER_MAX - 1] = '\0';
}

// 4) Affiche le contenu de messageInfo
void print_message(const MessageInfo *m) {
    printf("→ Expéditeur IP       : %s\n", m->sender_ip);
    printf("→ Destinataire pseudo : %s\n", m->dest_pseudo);
    printf("→ Partie              : %d / %d\n", m->part_num, m->total_parts);
    printf("→ Contenu             : %s\n\n", m->payload);
}

// 5) Envoie un accusé de réception à l’expéditeur
void send_ack(int sockfd, const struct sockaddr_in *cli) {
    const char *ack = "[ACK] reçu\n";
    socklen_t len = sizeof(*cli);
    if (sendto(sockfd, ack, strlen(ack), 0,
               (struct sockaddr*)cli, len) < 0) {
        perror("sendto");
    }
}

// -------- point d’entrée --------
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <taille_max_reception>\n", argv[0]);
        return EXIT_FAILURE;
    }
    int taille = atoi(argv[1]);
    if (taille <= 0 || taille > BUFFER_MAX) {
        fprintf(stderr, "Erreur : taille_max_reception doit être entre 1 et %d\n", BUFFER_MAX);
        return EXIT_FAILURE;
    }

    int sockfd = init_server_socket();
    char *buffer = malloc(taille);
    if (!buffer) { perror("malloc"); close(sockfd); return EXIT_FAILURE; }

    struct sockaddr_in client;
    while (1) {
        ssize_t r = recv_message(sockfd, buffer, &client);
        if (r < 0) break;

        MessageInfo msg;
        parse_message(buffer, &msg);
        print_message(&msg);
        send_ack(sockfd, &client);
    }

    free(buffer);
    return EXIT_SUCCESS;
}
