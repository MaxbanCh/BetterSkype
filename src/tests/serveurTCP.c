#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include "../dependencies/header.h"
#include "../dependencies/TCPFile.h"  // Inclure votre bibliothèque TCPFile

#define BUFFER_MAX 1024

static void handleSigint(int sig) {
    (void)sig;  
    printf("\nArrêt serveur\n");
    //close(sockfd); 
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    int socketServer, socketClient;
    char filename[256];
    
    // Vérifier les arguments
    if (argc != 2) {
        printf("Usage: %s <filename_to_save>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    strcpy(filename, argv[1]);
    
    // Gérer le signal SIGINT pour une fermeture propre
    signal(SIGINT, handleSigint);
    
    // Initialiser le serveur TCP
    socketServer = initTCPSocketServer();
    printf("En attente de connexion d'un client...\n");
    
    // Accepter la connexion d'un client
    socketClient = connexionTCP(socketServer);
    printf("Client connecté! En attente du fichier...\n");
    
    // Recevoir le fichier
    int result = receiveFile(socketClient, filename);
    if (result == 0) {
        printf("Fichier reçu avec succès et enregistré sous '%s'\n", filename);
    } else {
        printf("Erreur lors de la réception du fichier (code %d)\n", result);
    }
    
    // Fermer les sockets
    closeServer(socketServer, socketClient);
    
    return 0;

}