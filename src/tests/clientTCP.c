#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include "../dependencies/header.h"
#include <signal.h>
#include "../dependencies/TCPFile.h"  // Inclure votre bibliothèque TCPFile

#define BUFFER_MAX 1024

static void handleSigint(int sig) {
    (void)sig;  
    printf("\nArrêt serveur\n");
    //close(sockfd); 
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    int socketClient;
    
    // Vérifier les arguments
    if (argc != 3) {
        printf("Usage: %s <server_ip> <filename_to_send>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    char *serverIP = argv[1];
    char *filename = argv[2];
    
    // Vérifier que le fichier existe
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        file = fopen(filename, "wb");
        if (file == NULL) {
            perror("Erreur: impossible de créer le fichier");
            exit(EXIT_FAILURE);
        }
    }
    fclose(file);

    // Initialiser la connexion TCP au serveur
    printf("Connexion au serveur %s...\n", serverIP);
    socketClient = initTCPSocketClient(serverIP);
    printf("Connecté au serveur! Envoi du fichier '%s'...\n", filename);
    
    // Envoyer le fichier
    int result = sendFile(socketClient, filename);
    if (result == 0) {
        printf("Fichier envoyé avec succès!\n");
    } else {
        printf("Erreur lors de l'envoi du fichier (code %d)\n", result);
    }
    
    // Fermer la socket
    closeClient(socketClient);
    
    return 0;
}