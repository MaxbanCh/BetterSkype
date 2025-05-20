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


static void handleSigint() {
    // (void)sig;  
    printf("\nArrêt serveur\n");
    //close(sockfd); 
    exit(EXIT_SUCCESS);
}

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
    int dSC = accept(socket, (struct sockaddr*) &aC, &lg);

    if (dSC == -1) {
        perror("Erreur acceptation connexion");
        close(socket);
        exit(1);
    }
    
    printf("Client connecté depuis %s:%d\n", 
           inet_ntoa(aC.sin_addr), ntohs(aC.sin_port));

    return dSC;
}

int sendFile(int socket, char *file)
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
        if (ret <= 0 || ack != 'A' && flag == 0) {  // 'A' pour ACK
            fprintf(stderr, "Erreur: accusé de réception non reçu ou incorrect pour un bloc\n");
            fclose(fp);
            flag = -5;
        }
    }
    
    fclose(fp);
    return flag;  // Succès
}

int receiveFile(int socket, char *file)
{
    FILE *fp;
    char buffer[BUFFER_MAX];
    size_t bytesReceived;
    long fileSize;
    int ret;
    char ack = 'A'; // A pour ACK

    int flag = 0;

    // Vérifier si c'est un message d'erreur avec MSG_PEEK
    char errorCheck[15];
    ret = recv(socket, errorCheck, 6, MSG_PEEK);
    if (ret == 6 && strcmp(errorCheck, "ERROR:") == 0) {
        // C'est un message d'erreur, lire le message complet
        char errorFull[100];
        memset(errorFull, 0, sizeof(errorFull));
        recv(socket, errorFull, sizeof(errorFull)-1, 0);
        printf("Message d'erreur du client: %s\n", errorFull);
        flag = -10;  // Code spécial pour les erreurs client
        return flag;
    }

    // 1. Recevoir la taille du fichier
    ret = recv(socket, &fileSize, sizeof(fileSize), 0);
    if (ret <= 0) {
        perror("Erreur lors de la réception de la taille du fichier");
        flag = -1;
    }

    // Ouvrir le fichier en mode écriture binaire
    // N'ouvrir le fichier que si tout est bon jusqu'ici
    if (flag == 0) {
        fp = fopen(file, "wb");
        if (fp == NULL) {
            perror("Erreur lors de l'ouverture du fichier");
            flag = -2;
        }
    }
    
    // 2. Envoyer l'accusé de réception (ACK)
    if (flag == 0) {
        ret = send(socket, &ack, 1, 0);
        if (ret < 0) {
            perror("Erreur lors de l'envoi de l'accusé de réception");
            fclose(fp);
            flag = -3;
        }
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
    if (fp != NULL) {
        fclose(fp);
    }
    return flag;  // Succès
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