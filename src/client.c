// ------ Importation des Librairies ------

#include <sys/socket.h>
#include <netinet/in.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>  // Ajout pour gérer les threads
#include "message.h"
#include "header.h"
#include "client.h"
#include "dependencies/TCPFile.h"

// ------  Variables globales ------      
#define SERVER_PORT 12345

char SERVER_IP[INET_ADDRSTRLEN] = "127.0.0.1";  // IP par défaut
int clientRunning = 1; 
int globalSocket = -1;  // Socket globale pour la déconnexion
struct sockaddr_in *globalServer; // Adresse serveur globale

static void handleSigint(int sig) {
    (void)sig;
    printf("\nArrêt client\n");

     // Envoyer une demande de déconnexion si nous avons une socket valide
    if (globalSocket >= 0 && globalServer != NULL) {
        // Envoyer simplement @disconnect comme pour la commande manuelle
        char *message = createMessage(SERVER_IP, "", "@disconnect");
        sendMessage(globalSocket, globalServer, message);
        free(message);
        
        // Donner au serveur un peu de temps pour traiter la déconnexion
        usleep(100000);  // 100ms
    }
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
}

/* Fonction d'envoi d'un message
 *var dS :          Socket de connexion
 *var adServer :    Serveur destinataire
 *var message :     message formalise au prealable
*/
ssize_t sendMessage(int dS, struct sockaddr_in *adServer, char *message)
{
    socklen_t lgA = sizeof(struct sockaddr_in);
    return sendto(dS, message, strlen(message), 0, (struct sockaddr *)adServer, lgA);
}

// Fonction de debug d'envoi d'un message
void debugSendMessage(ssize_t snd)
{
    if (snd == -1)
        perror("Erreur envoi msg : ");
}

// Structure pour passer les paramètres au thread de transfert
typedef struct {
    char operation[16];
    char filename[256];
    char serverIP[64];
    char baseName[256];
} FileTransferThreadParams;

// Fonction exécutée par le thread pour gérer le transfert de fichier
void *fileTransferThread(void *arg) {
    FileTransferThreadParams *params = (FileTransferThreadParams *)arg;
    int result = -1;
    char fullPath[512] = {0};
    
    // Construire le chemin complet selon l'opération
    if (strcmp(params->operation, "UPLOAD") == 0) {
        snprintf(fullPath, sizeof(fullPath), "files/send/%s", params->baseName);
        
        // Vérifier que le fichier existe avant de l'envoyer
        FILE *testFile = fopen(fullPath, "r");
        if (!testFile) {
            char errorMsg[512];
            snprintf(errorMsg, sizeof(errorMsg), 
                    "\nErreur: Le fichier %s n'existe pas dans le dossier files/send/\n> ", 
                    params->baseName);
            write(STDOUT_FILENO, errorMsg, strlen(errorMsg));
            
            int socketTCP = initTCPSocketClient(params->serverIP);
            if (socketTCP >= 0) {
                // Envoyer message d'erreur au serveur
                char errorNotification[256] = "ERROR:FILE_NOT_FOUND";
                send(socketTCP, errorNotification, strlen(errorNotification), 0);
                closeClient(socketTCP);
            }
            
            free(params);
            pthread_exit(NULL);
        }
        
        fclose(testFile);
        
        // Établir connexion TCP et envoyer le fichier
        int socketTCP = initTCPSocketClient(params->serverIP);
        if (socketTCP < 0) {
            write(STDOUT_FILENO, "\nErreur de connexion TCP\n> ", 28);
            free(params);
            pthread_exit(NULL);
        }
        
        write(STDOUT_FILENO, "\nEnvoi du fichier en cours...\n", 30);
        result = sendFile(socketTCP, fullPath);
        
        if (result == 0) {
            char successMsg[512];
            snprintf(successMsg, sizeof(successMsg), 
                    "\nFichier %s envoyé avec succès depuis files/send/\n> ", params->baseName);
            write(STDOUT_FILENO, successMsg, strlen(successMsg));
        } else {
            char errorMsg[512];
            snprintf(errorMsg, sizeof(errorMsg), 
                    "\nErreur lors de l'opération fichier (code: %d)\n> ", result);
            write(STDOUT_FILENO, errorMsg, strlen(errorMsg));
        }
        
        closeClient(socketTCP);
    } 
    else if (strcmp(params->operation, "DOWNLOAD") == 0) {
        snprintf(fullPath, sizeof(fullPath), "files/receive/%s", params->baseName);
        
        // Établir connexion TCP et recevoir le fichier
        int socketTCP = initTCPSocketClient(params->serverIP);
        if (socketTCP < 0) {
            write(STDOUT_FILENO, "\nErreur de connexion TCP\n> ", 28);
            free(params);
            pthread_exit(NULL);
        }
        
        write(STDOUT_FILENO, "\nRéception du fichier en cours...\n", 34);
        result = receiveFile(socketTCP, fullPath);
        
        if (result == 0) {
            char successMsg[512];
            snprintf(successMsg, sizeof(successMsg), 
                    "\nFichier %s reçu avec succès dans files/receive/\n> ", params->baseName);
            write(STDOUT_FILENO, successMsg, strlen(successMsg));
        } else {
            char errorMsg[512];
            snprintf(errorMsg, sizeof(errorMsg), 
                    "\nErreur lors de l'opération fichier (code: %d)\n> ", result);
            write(STDOUT_FILENO, errorMsg, strlen(errorMsg));
        }
        
        closeClient(socketTCP);
    }
    
    free(params);
    pthread_exit(NULL);
}

// Fonction qui gère les transferts de fichiers via TCP (version avec thread)
void handleTCPFileTransfer(char *buffer) {

    int flag = 1; // 1 = continuer, 0 = ne pas continuer
    // Extraire les informations de la commande TCP reçue
    char operation[32], filename[256], ip[INET_ADDRSTRLEN];
    memset(operation, 0, sizeof(operation));
    memset(filename, 0, sizeof(filename));
    memset(ip, 0, sizeof(ip));

    pthread_t thread; // Créer un thread pour gérer le transfert
    const char *baseName ;
    FileTransferThreadParams *params = malloc(sizeof(FileTransferThreadParams));
    
    if (sscanf(buffer, "TCP:%31[^:]:%255[^:]:%46s", operation, filename, ip) != 3) {
        write(STDOUT_FILENO, "\nFormat de commande TCP invalide\n> ", 35);
        flag = 0;
    }
    
    // Extraire le nom de base du fichier (sans chemin)
    if (flag){
        baseName = strrchr(filename, '/');
        if (baseName) {
            baseName++; // Passer après le '/'
        } else {
            baseName = strrchr(filename, '\\');
            if (baseName) {
                baseName++; // Passer après le '\'
            } else {
                baseName = filename;
            }
        }
        
        if (!params) {
            write(STDOUT_FILENO, "\nErreur d'allocation mémoire\n> ", 32);
            flag = 0;
        }
    }
    if (flag){
        strncpy(params->operation, operation, sizeof(params->operation) - 1);
        strncpy(params->filename, filename, sizeof(params->filename) - 1);
        strncpy(params->serverIP, ip, sizeof(params->serverIP) - 1);
        strncpy(params->baseName, baseName, sizeof(params->baseName) - 1);
        
        
        if (pthread_create(&thread, NULL, fileTransferThread, params) != 0) {
            write(STDOUT_FILENO, "\nÉchec création thread de transfert\n> ", 40);
            free(params);
            flag = 0;
        }
    }
    // Détacher le thread pour qu'il se libère automatiquement à la fin
    if (flag){
        pthread_detach(thread);
    }
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        strncpy(SERVER_IP, argv[1], INET_ADDRSTRLEN - 1);
        SERVER_IP[INET_ADDRSTRLEN - 1] = '\0'; 
        printf("IP utilisée: %s\n", SERVER_IP);
    } else {
        printf("IP par défaut utilisée: %s\n", SERVER_IP);
        printf("Pour utiliser une IP spécifique: %s <adresse_ip>\n", argv[0]);
    }
    int dS = createSocket();
    if (dS < 0) {
        perror("socket");
        exit(EXIT_FAILURE);         // Cas d'erreur 
    }

    struct sockaddr_in *adServer = adServ(SERVER_PORT);

    // Stocker les références globales pour le gestionnaire SIGINT
    globalSocket = dS;
    globalServer = adServer;

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
            else if (strncmp(buf, "FILES:", 6) == 0) {
                // On ignore les messages FILES: pour éviter leur affichage
                write(STDOUT_FILENO, "\n> ", 3);
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