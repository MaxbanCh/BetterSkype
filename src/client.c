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
    return sendto(dS, message, strlen(message), 0, (struct sockaddr *)adServer, lgA);
}

// Fonction de debug d'envoi d'un message
void debugSendMessage(ssize_t snd)
{
    if (snd == -1)
        perror("Erreur envoi msg : ");

    return ;
}

// Fonction qui gère les transferts de fichiers via TCP
// Dans client.c
// Fonction qui gère les transferts de fichiers via TCP
void handleTCPFileTransfer(char *buffer) {
    // Variables pour contrôler le flux de la fonction
    int valid = 1;
    int result = -1;
    int socketTCP = -1;
    
    // Extraire les informations de la commande TCP reçue
    char operation[32], filename[256], ip[INET_ADDRSTRLEN];
    memset(operation, 0, sizeof(operation));
    memset(filename, 0, sizeof(filename));
    memset(ip, 0, sizeof(ip));
    
    const char *baseName = NULL;
    char fullPath[512] = {0};
    
    // Étape 1: Analyse du buffer pour extraire les informations
    if (sscanf(buffer, "TCP:%31[^:]:%255[^:]:%46s", operation, filename, ip) != 3) {
        write(STDOUT_FILENO, "Format de commande TCP invalide\n", 32);
        valid = 0;
    }
    
    // Étape 2: Extraction du nom de base du fichier si on continue
    if (valid) {
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
    }
    
    // Étape 3: Initialisation de la connexion TCP
    if (valid) {
        socketTCP = initTCPSocketClient(ip);
        if (socketTCP < 0) {
            write(STDOUT_FILENO, "Erreur de connexion TCP\n", 24);
            valid = 0;
        }
    }
    
    // Étape 4: Traitement des opérations selon le type
    if (valid) {
        if (strcmp(operation, "UPLOAD") == 0) {
            // Construction du chemin source pour l'upload
            snprintf(fullPath, sizeof(fullPath), "files/send/%s", baseName);
            
            // Vérifier que le fichier existe avant de l'envoyer
            FILE *testFile = fopen(fullPath, "r");
            if (!testFile) {
                char errorMsg[512];
                snprintf(errorMsg, sizeof(errorMsg), 
                        "Erreur: Le fichier %s n'existe pas dans le dossier files/send/\n", baseName);
                write(STDOUT_FILENO, errorMsg, strlen(errorMsg));
                
                // Envoyer un message d'erreur au serveur
                char errorNotification[256] = "ERROR:FILE_NOT_FOUND";
                send(socketTCP, errorNotification, strlen(errorNotification), 0);
                
                result = -1;
            }
            else {
                fclose(testFile);
                
                write(STDOUT_FILENO, "\nEnvoi du fichier en cours...\n", 30);
                result = sendFile(socketTCP, fullPath);
            }
        }
        else if (strcmp(operation, "DOWNLOAD") == 0) {
            // Construction du chemin destination pour le download
            snprintf(fullPath, sizeof(fullPath), "files/receive/%s", baseName);
            
            write(STDOUT_FILENO, "\nRéception du fichier en cours...\n", 34);
            result = receiveFile(socketTCP, fullPath);
        }
        else {
            write(STDOUT_FILENO, "Opération non reconnue\n", 24);
            valid = 0;
        }
    }
    
    // Étape 5: Affichage du résultat si l'opération est valide
    if (valid) {
        if (result == 0) {
            char successMsg[512];
            if (strcmp(operation, "UPLOAD") == 0) {
                snprintf(successMsg, sizeof(successMsg), 
                        "Fichier %s envoyé avec succès depuis files/send/\n", baseName);
            } else {
                snprintf(successMsg, sizeof(successMsg), 
                        "Fichier %s reçu avec succès dans files/receive/\n", baseName);
            }
            write(STDOUT_FILENO, successMsg, strlen(successMsg));
        } else {
            char errorMsg[512];
            snprintf(errorMsg, sizeof(errorMsg), 
                    "Erreur lors de l'opération fichier (code: %d)\n", result);
            write(STDOUT_FILENO, errorMsg, strlen(errorMsg));
        }
    }
    
    // Étape 6: Fermer la connexion TCP si elle a été ouverte
    if (socketTCP >= 0) {
        closeClient(socketTCP);
    }
    
    // Fin de la fonction sans return (inutile pour une fonction void)
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