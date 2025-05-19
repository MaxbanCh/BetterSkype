#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include "message.h"  // structure contenant les champs du message
#include "header.h"
#include "TCPFile.h"
#include <pthread.h>
#include <sys/time.h>  // Pour struct timeval
#include <errno.h>     // Pour les constantes d'erreur


// static void handleSigint() {
//     // (void)sig;  
//     printf("\nArrêt serveur\n");
//     //close(sockfd); 
//     exit(EXIT_SUCCESS);
// }

int initTCPSocketServer() {
    int fd = socket(PF_INET, SOCK_STREAM, 0); 
    if (fd < 0) { perror("socket"); exit(EXIT_FAILURE); }

    int option = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
        perror("setsockopt(SO_REUSEADDR)");
        close(fd);
        return -1;
    }

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,         // famille d'adresses : IPv4
        .sin_port        = htons(34567),     // port d'écoute
        .sin_addr.s_addr = INADDR_ANY       // écoute sur toutes les interfaces
    };

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); 
        close(fd); 
        exit(EXIT_FAILURE);
    }

    if (listen(fd, 7) == -1) {
        perror("Erreur lors de la mise en écoute");
        close(fd);
        exit(1);
    }

    printf("Serveur TCP démarré sur le port %d\n", 34567);
    return fd; 
}   

int initTCPSocketClient(char *ip) {
    int fd = socket(PF_INET, SOCK_STREAM, 0); 
    if (fd < 0) { perror("socket"); exit(EXIT_FAILURE); }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;         // famille d'adresses : IPv4
    addr.sin_port = htons(34567);       // port d'écoute


    int res = inet_pton(AF_INET, ip, &(addr.sin_addr));
    socklen_t lgA = sizeof(struct sockaddr_in);

    res = connect(fd, (struct sockaddr*)&addr, lgA);

    if (res < 0) {
        perror("Erreur de connexion");
        close(fd);
        exit(1);
    }

    return fd; 
}

int connexionTCP(int socket)
{
    struct sockaddr_in aC;
    socklen_t lg = sizeof(struct sockaddr_in);
    
    // Configurer un timeout pour accept()
    struct timeval tv;
    tv.tv_sec = 10;  // 10 secondes d'attente maximum
    tv.tv_usec = 0;
    
    // Appliquer le timeout à la socket
    if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) < 0) {
        perror("Erreur lors de la configuration du timeout");
        return -1;
    }
    
    printf("En attente de connexion TCP (timeout: 10s)...\n");
    int dSC = accept(socket, (struct sockaddr*) &aC, &lg);

    if (dSC == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Timeout atteint
            printf("Timeout: aucun client ne s'est connecté dans les 10 secondes\n");
            return -2;  // Code spécifique pour le timeout
        } else {
            // Autre erreur
            perror("Erreur acceptation connexion");
            return -1;  // Ne pas terminer le programme, juste retourner une erreur
        }
    }
    
    printf("Client connecté depuis %s:%d\n", 
           inet_ntoa(aC.sin_addr), ntohs(aC.sin_port));

    return dSC;
}

int sendFile(int socket, const char *file)
{
    FILE *fp;
    char buffer[BUFFER_MAX];
    size_t bytesRead;
    long fileSize;
    int ret;

    int flag = 0;
    
    // Ouvrir le fichier en mode lecture binaire
    fp = fopen(file, "rb");
    if (fp == NULL) {
        perror("Erreur lors de l'ouverture du fichier");
        fileSize = -1;  // Utiliser -1 comme taille de fichier pour signaler une erreur
        send(socket, &fileSize, sizeof(fileSize), 0);
        flag = -1;
    }
    
    // Obtenir la taille du fichier
    fseek(fp, 0, SEEK_END);
    fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // 1. Envoyer la taille du fichier
    ret = send(socket, &fileSize, sizeof(fileSize), 0);
    if (ret < 0 && flag == 0) {
        perror("Erreur lors de l'envoi de la taille du fichier");
        fclose(fp);
        flag = -2;
    }
    
    // 2. Attendre l'accusé de réception du client (ACK)
    char ack;
    ret = recv(socket, &ack, 1, 0);
    if ((ret <= 0 || ack != 'A') && flag == 0) {  // 'A' pour ACK
        fprintf(stderr, "Erreur: accusé de réception non reçu ou incorrect\n");
        fclose(fp);
        flag = -3;
    }

    // 3. Envoyer le contenu du fichier par blocs
    while ((bytesRead = fread(buffer, 1, BUFFER_MAX, fp)) > 0 && flag == 0) {
        // Envoyer le bloc
        ret = send(socket, buffer, bytesRead, 0);
        if (ret < 0 && flag == 0) {
            perror("Erreur lors de l'envoi du bloc de données");
            fclose(fp);
            flag = -4;
        }
        
        // Attendre l'accusé de réception pour ce bloc
        ret = recv(socket, &ack, 1, 0);
        if (ret <= 0 || (ack != 'A' && flag == 0)) {  // 'A' pour ACK
            fprintf(stderr, "Erreur: accusé de réception non reçu ou incorrect pour un bloc\n");
            fclose(fp);
            flag = -5;
        }
    }
    
    fclose(fp);
    return flag;  // Succès
}

int receiveFile(int socket, const char *file)
{
    FILE *fp;
    char buffer[BUFFER_MAX];
    size_t bytesReceived;
    long fileSize;
    int ret;

    int flag = 0;

    // 1. Recevoir la taille du fichier
    ret = recv(socket, &fileSize, sizeof(fileSize), 0);
    if (ret <= 0) {
        perror("Erreur lors de la réception de la taille du fichier");
        flag = -1;
    }

    printf("Taille du fichier à recevoir : %ld octets\n", fileSize);
    if (fileSize == -1) {
        printf("Le client a signalé une erreur avec le fichier source\n");
        flag = -1;
    }

    // Ouvrir le fichier en mode écriture binaire
    fp = fopen(file, "wb");
    if (fp == NULL && flag == 0) {
        perror("Erreur lors de l'ouverture du fichier");
        flag = -2;
    }

    // 2. Envoyer l'accusé de réception (ACK)
    char ack = 'A';
    ret = send(socket, &ack, 1, 0);
    if (ret < 0 && flag == 0) {
        perror("Erreur lors de l'envoi de l'accusé de réception");
        fclose(fp);
        flag = -3;
    }

    // 3. Recevoir le contenu du fichier par blocs
    while (fileSize > 0 && flag == 0) {
        bytesReceived = recv(socket, buffer, BUFFER_MAX, 0);
        if (bytesReceived <= 0 && flag == 0) {
            perror("Erreur lors de la réception du bloc de données");
            fclose(fp);
            flag = -4;
        }

        // Écrire le bloc dans le fichier
        fwrite(buffer, 1, bytesReceived, fp);
        fileSize -= bytesReceived;

        // Envoyer l'accusé de réception pour ce bloc
        ret = send(socket, &ack, 1, 0);
        if (ret < 0 && flag == 0) {
            perror("Erreur lors de l'envoi de l'accusé de réception pour un bloc");
            fclose(fp);
            flag = -5;
        }
    }

    fclose(fp);
    return flag;  // Succès
}


void *fileTransferThreadServer(void *arg) {
    FileTransferParams *params = (FileTransferParams *)arg;
    int result = 0;
    int socketTCP = params->socketTCP;
    
    printf("Thread démarré pour %s du fichier %s\n", 
           params->operation, params->filename);
    
    int clientTCP = connexionTCP(socketTCP);
    if (clientTCP < 0) {
        if (clientTCP == -2) {
            printf("Aucun client ne s'est connecté dans le délai imparti.\n");
            printf("Probablement car le fichier n'existe pas côté client ou autre erreur.\n");
        }
        else {
            printf("Échec connexion TCP avec le client\n");
        }
        closeServer(socketTCP, -1);
        free(params);
        pthread_exit(NULL);
        return NULL;  // Retourne NULL en cas d'échec
    }

    // Ajouter un petit délai pour s'assurer que la connexion est établie
    sleep(1);
    
    if (strcmp(params->operation, "@upload") == 0) {
        // Client upload = serveur reçoit
        printf("Réception du fichier %s en cours...\n", params->filename);
        result = receiveFile(clientTCP, params->filename);
        if (result == -2) {
            printf("Le client a indiqué que le fichier source n'existe pas\n");
        } else if (result == 0) {
            printf("Fichier reçu avec succès\n");
        } else {
            printf("Erreur lors de la réception du fichier (code %d)\n", result);
        }
    } else if (strcmp(params->operation, "@download") == 0) {
        // Client download = serveur envoie
        printf("Envoi du fichier %s en cours...\n", params->filename);
        result = sendFile(clientTCP, params->filename);
        if (result == 0) {
            printf("Fichier envoyé avec succès\n");
        } else {
            printf("Erreur lors de l'envoi du fichier (code %d)\n", result);
        }
    }
    
    // Fermer les connexions TCP pour ce transfert
    printf("Fermeture des connexions TCP\n");
    closeServer(socketTCP, clientTCP);
    free(params);
    pthread_exit(NULL);
    return NULL;  // Cette ligne ne sera jamais exécutée mais élimine l'avertissement
}

void closeServer(int socket, int socketClient)
{
	close(socket);
	close(socketClient);
}

void closeClient(int socket)
{
	close(socket);
}