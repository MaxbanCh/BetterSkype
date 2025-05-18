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
#include "dependencies/TCPFile.h"  // Ajout de l'inclusion


// ------  Variables globales ------
#define SERVER_IP   "127.0.0.1"         
#define SERVER_PORT 12345

int clientRunning = 1; 

static void handleSigint(int sig) {
    (void)sig;
    printf("\nArrêt client\n");
    clientRunning = 0;
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
        printf("BIENVENUE SUR BETTERSKYPE !!!!!!\n");
    else
        printf("Ohhhh nonnnnn, il semble qu'il y'a une erreur\n");

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

// Fonction qui gère les transferts de fichiers via TCP
void handleTCPFileTransfer(char *buffer) {
    // Format attendu: "TCP:<operation>:<fichier>:<ip_serveur>"
    // où operation est soit "SEND" soit "RECEIVE"
    
    char *operation, *filename, *serverIP;
    
    // Extraire les informations du message
    operation = strtok(buffer + 4, ":");  // +4 pour sauter "TCP:"
    if (!operation) {
        write(STDOUT_FILENO, "\nFormat TCP invalide\n> ", 24);
        return;
    }
    
    filename = strtok(NULL, ":");
    if (!filename) {
        write(STDOUT_FILENO, "\nNom de fichier manquant\n> ", 28);
        return;
    }
    
    serverIP = strtok(NULL, ":");
    if (!serverIP) {
        // Si l'IP n'est pas spécifiée, utiliser celle par défaut
        serverIP = SERVER_IP;
    }
    
    write(STDOUT_FILENO, "\nInitialisation transfert TCP...\n", 33);
    
    // Initialiser la connexion TCP
    int socketTCP = initTCPSocketClient(serverIP);
    if (socketTCP < 0) {
        write(STDOUT_FILENO, "\nÉchec connexion TCP\n> ", 25);
        return;
    }
    
    int result;
    if (strcmp(operation, "UPLOAD") == 0) {
        // Envoyer le fichier
        write(STDOUT_FILENO, "\nEnvoi du fichier en cours...\n", 30);
        result = sendFile(socketTCP, filename);
        if (result == 0) {
            write(STDOUT_FILENO, "\nFichier envoyé avec succès\n> ", 31);
        } else {
            char error[50];
            sprintf(error, "\nErreur lors de l'envoi du fichier (code %d)\n> ", result);
            write(STDOUT_FILENO, error, strlen(error));
        }
    } 
    else if (strcmp(operation, "DOWNLOAD") == 0) {
        // Recevoir le fichier
        write(STDOUT_FILENO, "\nRéception du fichier en cours...\n", 34);
        result = receiveFile(socketTCP, filename);
        if (result == 0) {
            write(STDOUT_FILENO, "\nFichier reçu avec succès\n> ", 30);
        } else {
            char error[60];
            sprintf(error, "\nErreur lors de la réception du fichier (code %d)\n> ", result);
            write(STDOUT_FILENO, error, strlen(error));
        }
    }
    else {
        write(STDOUT_FILENO, "\nOpération TCP non reconnue\n> ", 31);
    }
    
    // Fermer la connexion TCP
    closeClient(socketTCP);
}


int main(void) {
    
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
        signal(SIGINT, handleSigint);
        // ── Processus enfant : réception bloquante des ACKs ──
        char buf[BUFFER_MAX];
        while (clientRunning) {
            ssize_t r = recvfrom(dS, buf, BUFFER_MAX-1, 0, NULL, NULL);
            if (r < 0) {
                perror("recvfrom"); 
                _exit(EXIT_FAILURE);
            }
            buf[r] = '\0';

            // Gestion accusé de réception
            if (strcmp(buf, "OK") == 0) {
                write(STDOUT_FILENO, "\nACK reçu\n> ", 12);
            } 
            else if (strncmp(buf, "TCP", 3) == 0) {
                handleTCPFileTransfer(buf);
            }
            else {
                write(STDOUT_FILENO, "\n", 1);
                write(STDOUT_FILENO, buf, r);
                write(STDOUT_FILENO, "\n> ", 3);
            }

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
            char *message;
            char dest[PSEUDO_MAX];
            memset(dest, 0, sizeof dest);
            // Si la commande commence par "@msg", on récupère le destinataire
            if (sscanf(line, "@msg %s", dest) == 1) {
                // dest contient le pseudo cible
                message = createMessage(SERVER_IP, dest, line);
            } else {
                // pour les autres commandes (broadcast ou authentification), on peut laisser dest vide
                message = createMessage(SERVER_IP, "", line);
            }
            ssize_t s = sendMessage(dS, adServer, message);
            debugSendMessage(s);
            free(message);

        }

        kill(pid, SIGTERM);
        wait(NULL);
        close(dS);
        free(adServer);
    }

    return EXIT_SUCCESS;
}