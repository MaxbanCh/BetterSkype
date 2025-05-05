#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "MessageInfo.h"  

#define PORT         12345
#define BUFFER_MAX   1024
#define FIELD_DELIM   "//"
#define PAYLOAD_DELIM "/#/#00"


int   init_server_socket(void);
ssize_t recv_message(int sockfd, char *buffer, size_t buflen, struct sockaddr_in *cli);
int   parse_message(const char *buffer, MessageInfo *msg);
void  print_message(const MessageInfo *msg);
void  send_ack(int sockfd, const struct sockaddr_in *cli);

int main(void) {
    int sockfd = init_server_socket();

    char buffer[BUFFER_MAX]; 
    struct sockaddr_in client;

    while (1) {
        ssize_t r = recv_message(sockfd, buffer, sizeof(buffer), &client);
        if (r < 0) break;

        MessageInfo msg;
        if (parse_message(buffer, &msg) == 0) {
            print_message(&msg);
        } else {
            fprintf(stderr, "[WARN] parsing failed: %s\n", buffer);
        }

        send_ack(sockfd, &client);
    }
    
    return EXIT_SUCCESS;
}

// crée et bind la socket UDP
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

// reçoit sans jamais dépasser buflen-1, ajoute '\0'
ssize_t recv_message(int sockfd, char *buffer, size_t buflen,
                     struct sockaddr_in *cli) {
    socklen_t len = sizeof(*cli);
    ssize_t r = recvfrom(sockfd, buffer, buflen - 1, 0,
                         (struct sockaddr*)cli, &len);
    if (r < 0) {
        perror("recvfrom");
        return -1;
    }
    buffer[r] = '\0';
    return r;
}

int parse_message(const char *buffer, MessageInfo *msg) {
    const char *p = buffer, *sep;
    size_t len;
    char tmp[16];

    // 1) sender_ip
    sep = strstr(p, FIELD_DELIM);
    if (!sep) return -1;
    len = sep - p;
    memcpy(msg->sender_ip, p, len);
    msg->sender_ip[len] = '\0';

    // 2) dest_pseudo
    p = sep + strlen(FIELD_DELIM);
    sep = strstr(p, FIELD_DELIM);
    if (!sep) return -1;
    len = sep - p;
    memcpy(msg->dest_pseudo, p, len);
    msg->dest_pseudo[len] = '\0';

    // 3) part_num
    p = sep + strlen(FIELD_DELIM);
    sep = strstr(p, FIELD_DELIM);
    if (!sep) return -1;
    len = sep - p;
    memcpy(tmp, p, len); tmp[len] = '\0';
    msg->part_num = atoi(tmp);

    // 4) total_parts
    p = sep + strlen(FIELD_DELIM);
    sep = strstr(p, PAYLOAD_DELIM);      // <— on cherche maintenant "/#/#00"
    if (!sep) return -1;
    len = sep - p;
    memcpy(tmp, p, len); tmp[len] = '\0';
    msg->total_parts = atoi(tmp);

    // payload (tout ce qui suit PAYLOAD_DELIM)
    p = sep + strlen(PAYLOAD_DELIM);
    strncpy(msg->payload, p, sizeof(msg->payload)-1);
    msg->payload[sizeof(msg->payload)-1] = '\0';

    return 0;
}

// affichage
void print_message(const MessageInfo *m) {
    printf("→ Expéditeur IP       : %s\n", m->sender_ip);
    printf("→ Destinataire pseudo : %s\n", m->dest_pseudo);
    printf("→ Partie              : %d / %d\n", m->part_num, m->total_parts);
    printf("→ Contenu             : %s\n\n", m->payload);
}

// accusé de réception
void send_ack(int sockfd, const struct sockaddr_in *cli) {
    const char *ack = "reçu\n";
    socklen_t len = sizeof(*cli);
    if (sendto(sockfd, ack, strlen(ack), 0,
               (const struct sockaddr*)cli, len) < 0) {
        perror("sendto");
    }
}