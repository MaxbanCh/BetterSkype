// recepteur_parse.c – Serveur UDP multi-client avec parsing “//”
// Champ 1 : IP expéditeur
// Champ 2 : pseudo destinataire
// Champ 3 : n° de la partie du message
// Champ 4 : nombre total de parties
// Tout le reste après le 4ᵉ “//” est considéré comme payload

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT           12345
#define BUFFER_MAX     1024
#define PSEUDO_MAX     50
#define DELIM          "//"

// Structure pour stocker les champs extraits
typedef struct {
    char sender_ip[INET_ADDRSTRLEN];
    char dest_pseudo[PSEUDO_MAX];
    int  part_num;
    int  total_parts;
    char payload[BUFFER_MAX];
} MessageInfo;

// -------- Prototypes --------
int   init_server_socket(void);
char* alloc_buffer(int taille);
ssize_t recv_message(int sockfd, char *buffer, struct sockaddr_in *cli);
int   parse_buffer(const char *buffer, MessageInfo *msg);
void  print_message(const MessageInfo *msg);
void  send_ack(int sockfd, struct sockaddr_in *cli);

// -------- Implémentations --------

// 1) Crée et bind une socket UDP sur PORT
int init_server_socket(void) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { perror("socket"); exit(EXIT_FAILURE); }
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };
    if (bind(fd, (void*)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(fd); exit(EXIT_FAILURE);
    }
    printf("[INFO] Serveur démarré sur le port %d (UDP)\n", PORT);
    return fd;
}

// 2) Alloue un buffer de réception
char* alloc_buffer(int taille) {
    if (taille <= 0 || taille > BUFFER_MAX) {
        fprintf(stderr, "Erreur : taille_max_reception doit être entre 1 et %d\n", BUFFER_MAX);
        exit(EXIT_FAILURE);
    }
    char *b = malloc(taille);
    if (!b) { perror("malloc"); exit(EXIT_FAILURE); }
    return b;
}

// 3) Reçoit un datagramme UDP et retourne la longueur reçue
ssize_t recv_message(int sockfd, char *buffer, struct sockaddr_in *cli) {
    socklen_t len = sizeof(*cli);
    ssize_t r = recvfrom(sockfd, buffer, BUFFER_MAX-1, 0,
                         (struct sockaddr*)cli, &len);
    if (r < 0) perror("recvfrom");
    else buffer[r] = '\0';
    return r;
}

// 4) Parse buffer en 4 champs + payload
int parse_buffer(const char *buffer, MessageInfo *msg) {
    // On fait des copies successives des pointeurs
    const char *p = buffer;
    const char *sep;
    size_t  len;

    // Champ 1 : sender_ip
    sep = strstr(p, DELIM);
    if (!sep) return -1;
    len = sep - p;
    if (len >= sizeof(msg->sender_ip)) return -1;
    memcpy(msg->sender_ip, p, len);
    msg->sender_ip[len] = '\0';

    // Champ 2 : dest_pseudo
    p = sep + strlen(DELIM);
    sep = strstr(p, DELIM);
    if (!sep) return -1;
    len = sep - p;
    if (len >= sizeof(msg->dest_pseudo)) return -1;
    memcpy(msg->dest_pseudo, p, len);
    msg->dest_pseudo[len] = '\0';

    // Champ 3 : part_num
    p = sep + strlen(DELIM);
    sep = strstr(p, DELIM);
    if (!sep) return -1;
    len = sep - p;
    char tmp[16];
    if (len >= sizeof(tmp)) return -1;
    memcpy(tmp, p, len);
    tmp[len] = '\0';
    msg->part_num = atoi(tmp);

    // Champ 4 : total_parts
    p = sep + strlen(DELIM);
    sep = strstr(p, DELIM);
    if (!sep) return -1;
    len = sep - p;
    if (len >= sizeof(tmp)) return -1;
    memcpy(tmp, p, len);
    tmp[len] = '\0';
    msg->total_parts = atoi(tmp);

    // Le reste est le payload
    p = sep + strlen(DELIM);
    strncpy(msg->payload, p, sizeof(msg->payload)-1);
    msg->payload[sizeof(msg->payload)-1] = '\0';

    return 0;
}

// 5) Affiche joliment tous les champs
void print_message(const MessageInfo *m) {
    printf("→ Expéditeur IP     : %s\n", m->sender_ip);
    printf("→ Destinataire pseudo : %s\n", m->dest_pseudo);
    printf("→ Partie            : %d / %d\n", m->part_num, m->total_parts);
    printf("→ Contenu           : %s\n\n", m->payload);
}

// 6) Envoie un accusé de réception à l’expéditeur (echo simple)
void send_ack(int sockfd, struct sockaddr_in *cli) {
    const char *ack = "reçu ! \n";
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
    int sockfd = init_server_socket();
    char *buffer = alloc_buffer(taille);

    struct sockaddr_in client;
    while (1) {
        ssize_t r = recv_message(sockfd, buffer, &client);
        if (r <= 0) break;

        MessageInfo msg;
        if (parse_buffer(buffer, &msg) == 0) {
            print_message(&msg);
        } else {
            fprintf(stderr, "[WARN] impossible de parser : %s\n", buffer);
        }

        send_ack(sockfd, &client);
    }

    free(buffer);
    close(sockfd);
    return EXIT_SUCCESS;
}
