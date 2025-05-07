// ------ Importation des Librairies ------

#include <sys/socket.h>
#include <netinet/in.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include "message.h"
#include "header.h"
#include "client.h"


// ------  Variables globales ------
#define SERVER_IP   "127.0.0.1"         
#define SERVER_PORT 12345
static void handleSigint(int sig) {
    (void)sig;
    printf("\nArrêt client\n");
    exit(EXIT_SUCCESS);
}

// Fonction de creation de la socket UDP
int createSocket()
{
    return socket(PF_INET, SOCK_DGRAM, 0);
}

// Gestion du port utilise sur le programme 
struct sockaddr_in *adServ(int port)
{
    struct sockaddr_in *adServer = malloc(sizeof(struct sockaddr_in));
    adServer->sin_family = AF_INET; 
    adServer->sin_port = htons(port);

    return adServer;
}

// Fonction de connexion au serveur
int connection(int ds, char *ip, struct sockaddr_in *adServ)
{
    int res = inet_pton(AF_INET, ip, &(adServ->sin_addr));
    socklen_t lgA = sizeof(struct sockaddr_in);

    res = connect(ds, (struct sockaddr *) adServ, lgA);

    return res;
}

// Verification de la connexion
void debugConnexion(int res)
{
    if (res == 0)
        printf("ouais c'est connecte\n");
    else
        printf("nope dommage\n");

    return ;
}

/* Fonction d'envoi d'un message
 *var dS :          Socket de connexion
 *var adServer :    Serveur destinataire
 *var message :     message formalise au prealable
*/
ssize_t sendMessage(int dS, struct sockaddr_in *adServer, char *message)
{
    socklen_t lgA = sizeof(struct sockaddr_in);
    return sendto(dS, message, 1024, 0, (struct sockaddr *)adServer, lgA);
}

// Fonction de debug d'envoi d'un message
void debugSendMessage(ssize_t snd)
{
    if (snd == -1)
        perror("Erreur envoi msg : ");

    return ;
}

int main(void) {
    signal(SIGINT, handleSigint);
    int dS = createSocket();
    if (dS < 0) {
        perror("socket");
        exit(EXIT_FAILURE);         // Cas d'erreur 
    }

    struct sockaddr_in *adServer = adServ(SERVER_PORT);

    if ( connection(dS, SERVER_IP, adServer) != 0 ) {
        debugConnexion(-1);
        exit(EXIT_FAILURE);         // Cas d'erreur lors de la connexion
    }
    debugConnexion(0);

    pid_t pid = fork();             // Creation de processus

    if (pid < 0) {
        perror("fork"); 
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // ── Processus enfant : réception bloquante des ACKs ──
        char buf[BUFFER_MAX];
        while (1) {
            ssize_t r = recvfrom(dS, buf, BUFFER_MAX-1, 0, NULL, NULL);
            if (r < 0) {
                perror("recvfrom"); 
                _exit(EXIT_FAILURE);
            }
            buf[r] = '\0';
            write(STDOUT_FILENO, "\n ", 3);
            write(STDOUT_FILENO, buf, r);
            write(STDOUT_FILENO, "\n> ", 3);
        }
    } else {
        // ── Processus parent : envoi bloquant via read() ──
        char line[BUFFER_MAX];
        write(STDOUT_FILENO, "> ", 2);

        ssize_t n;
        while ((n = read(STDIN_FILENO, line, BUFFER_MAX-1)) > 0) {
            if (line[n-1] == '\n') n--;
            line[n] = '\0';

            // construction et envoi du message structuré
            char *message = createMessage(SERVER_IP, "destPseudo", line);
            ssize_t s = sendMessage(dS, adServer, message);
            debugSendMessage(s);
            free(message);

            write(STDOUT_FILENO, "> ", 2);
        }

        kill(pid, SIGTERM);
        wait(NULL);
        close(dS);
        free(adServer);
    }

    return EXIT_SUCCESS;
}