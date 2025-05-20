#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef _WIN32 
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <unistd.h>
#endif
#include "message.h"  // structure contenant les champs du message
#include "serveur.h"      
#include <signal.h>
#include "header.h"  
#include "user.h"
#include "command.h"
#include "dependencies/TCPFile.h"       
#include <pthread.h>  // Ajouté pour gérer les threads

int serverRunning = 1;
int sockfd; // Socket globale pour pouvoir la fermer avec ctrl+c

// Structure pour passer les paramètres au thread de transfert
typedef struct {
    char *operation;
    char *filename;
    struct sockaddr_in client;
    int socketTCP;
} FileTransferParams;

// Handler de signal SIGINT (Ctrl+C) pour arrêter proprement le serveur
static void handleSigint(int sig) {
    (void)sig;  
    printf("\nArrêt serveur\n");

    close(sockfd);
    serverRunning = 0;
    printf("Serveur arrêté.\n");
    exit(EXIT_SUCCESS);
}

// Fonction qui initialise la socket UDP et effectue le bind
int initSocket(void) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0); 
    if (fd < 0) { perror("socket"); exit(EXIT_FAILURE); }

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,         // famille d'adresses : IPv4
        .sin_port        = htons(PORT),     // port d'écoute
        .sin_addr.s_addr = INADDR_ANY       // écoute sur toutes les interfaces
    };

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); 
        close(fd); 
        exit(EXIT_FAILURE);
    }
    printf("Serveur UDP démarré sur le port %d\n", PORT);
    return fd; 
}

// Fonction qui reçoit un message depuis un client UDP
ssize_t recvMessage(int sockfd, char *buffer, size_t buflen, struct sockaddr_in *cli) {
    socklen_t len = sizeof(*cli);  // longueur de l'adresse client
    ssize_t result;
    ssize_t r = recvfrom(sockfd, buffer, buflen - 1, 0, (struct sockaddr*)cli, &len); // réception
    if (r < 0) {
        perror("recvfrom");
        result = -1;
    }
    else{
        buffer[r] = '\0'; 
        result = r;  
    }
    return result;
}

void printMessage(const MessageInfo *m) {
    printf("Expéditeur IP       : %s\n", m->sender_ip);
    printf("Destinataire pseudo : %s\n", m->dest_pseudo);
    printf("Partie              : %d / %d\n", m->part_num, m->total_parts);
    printf("Contenu             : %s\n\n", m->payload);
}

// Fonction qui envoie un accusé de réception à l'expéditeur du message
void sendAcR(int sockfd, const struct sockaddr_in *cli) {
    const char *acR = "OK"; 
    socklen_t len = sizeof(*cli);
    if (sendto(sockfd, acR, strlen(acR), 0, (const struct sockaddr*)cli, len) < 0) {
        perror("sendto"); 
    }
}

// Fonction exécutée dans un thread pour gérer un transfert de fichier
void *fileTransferThreadServer(void *arg) {
    FileTransferParams *params = (FileTransferParams *)arg;
    int result = -1;
    char filepath[512] = {0};
    
    // Extraire le nom de base du fichier sans chemin
    const char *baseName = strrchr(params->filename, '/');
    if (baseName) {
        baseName++; // Passer après le '/'
    } else {
        baseName = strrchr(params->filename, '\\');
        if (baseName) {
            baseName++; // Passer après le '\'
        } else {
            baseName = params->filename; // Pas de séparateur trouvé
        }
    }
    
    printf("Thread #%ld: En attente d'une connexion TCP...\n", pthread_self());
    
    // Attendre la connexion du client
    int clientTCP = connexionTCP(params->socketTCP);
    if (clientTCP < 0)  {
        printf("Thread #%ld: Échec connexion TCP avec le client\n", pthread_self());
        closeServer(params->socketTCP, -1);
        free(params->operation);
        free(params->filename);
        free(params);
        pthread_exit(NULL);
    }
    
    printf("Thread #%ld: Client connecté depuis %s:%d\n", 
           pthread_self(),
           inet_ntoa(params->client.sin_addr),
           ntohs(params->client.sin_port));
    
    // Déterminer l'opération et construire le chemin approprié
    if (strcmp(params->operation, "@upload") == 0) {
        // Le client veut uploader, le serveur va recevoir dans files/send
        snprintf(filepath, sizeof(filepath), "files/send/%s", baseName);
        printf("Thread #%ld: Réception du fichier %s en cours...\n", pthread_self(), filepath);
        
        // Réception du fichier
        result = receiveFile(clientTCP, filepath);
        if (result == 0) {
            printf("Thread #%ld: Fichier reçu avec succès dans %s\n", pthread_self(), filepath);
        } else {
            printf("Thread #%ld: Erreur lors de la réception du fichier (code %d)\n", pthread_self(), result);
        }
    } 
    else if (strcmp(params->operation, "@download") == 0) {
        // Le client veut télécharger, le serveur va envoyer depuis files/send
        snprintf(filepath, sizeof(filepath), "files/send/%s", baseName);
        printf("Thread #%ld: Envoi du fichier %s en cours...\n", pthread_self(), filepath);
        
        // Envoi du fichier
        result = sendFile(clientTCP, filepath);
        if (result == 0) {
            printf("Thread #%ld: Fichier envoyé avec succès depuis %s\n", pthread_self(), filepath);
        } else {
            printf("Thread #%ld: Erreur lors de l'envoi du fichier (code %d)\n", pthread_self(), result);
        }
    }
    
    // Fermer les connexions TCP
    closeServer(params->socketTCP, clientTCP);
    
    // Libérer la mémoire allouée
    free(params->operation);
    free(params->filename);
    free(params);
    
    pthread_exit(NULL);
}

int handleFileTransfer(const MessageInfo *msg, int sockfd, const struct sockaddr_in *client) {
    // Format attendu: "@upload <fichier>" ou "@download <fichier>"
    printf("Traitement du transfert de fichier...\n");
    
    char command[BUFFER_MAX];
    strncpy(command, msg->payload, sizeof(command) - 1);
    command[sizeof(command) - 1] = '\0';
    
    char *operation, *filename;
    
    // Extraire les informations du message
    operation = strtok(command, " ");
    if (!operation) {
        printf("Format de commande invalide\n");
        return -1;
    }
    
    filename = strtok(NULL, " ");
    if (!filename) {
        printf("Nom de fichier manquant\n");
        return -1;
    }
    
    // Extraire le nom de base du fichier sans chemin
    const char *baseName = strrchr(filename, '/');
    if (baseName) {
        baseName++; // Passer après le '/'
    } else {
        baseName = strrchr(filename, '\\');
        if (baseName) {
            baseName++; // Passer après le '\'
        } else {
            baseName = filename; // Pas de séparateur trouvé
        }
    }
    
    printf("Initialisation transfert de fichier avec %s...\n", inet_ntoa(client->sin_addr));
    
    // Déterminer l'opération et construire le chemin approprié
    char clientOperation[10];
    char filepath[512];  // Pour stocker le chemin complet
    
    if (strcmp(operation, "@upload") == 0) {
        // Le client veut uploader, le serveur va recevoir dans files/send
        strcpy(clientOperation, "UPLOAD");
        snprintf(filepath, sizeof(filepath), "files/send/%s", baseName);
        printf("Fichier sera reçu dans: %s\n", filepath);
    } else if (strcmp(operation, "@download") == 0) {
        // Le client veut télécharger, le serveur va envoyer depuis files/send
        strcpy(clientOperation, "DOWNLOAD");
        snprintf(filepath, sizeof(filepath), "files/send/%s", baseName);
        printf("Fichier sera envoyé depuis: %s\n", filepath);
    } else {
        printf("Opération non reconnue: %s\n", operation);
        return -1;
    }
    
    // Envoi d'une commande TCP au client pour l'informer qu'une connexion TCP va être établie
    char tcpResponse[BUFFER_MAX];
    snprintf(tcpResponse, sizeof(tcpResponse), "TCP:%s:%s:%s", clientOperation, 
             baseName, inet_ntoa(client->sin_addr));
    
    socklen_t len = sizeof(*client);
    if (sendto(sockfd, tcpResponse, strlen(tcpResponse), 0, (const struct sockaddr*)client, len) < 0) {
        perror("Erreur envoi commande TCP");
        return -1;
    }
    
    printf("Commande TCP envoyée: %s\n", tcpResponse);
    
    // Initialiser le mode serveur TCP 
    printf("Démarrage du serveur TCP...\n");
    int socketTCP = initTCPSocketServer();
    if (socketTCP < 0) {
        printf("Échec initialisation serveur TCP\n");
        return -1;
    }
    
    // Préparer les paramètres pour le thread de transfert
    FileTransferParams *params = malloc(sizeof(FileTransferParams));
    if (!params) {
        perror("Allocation mémoire pour les paramètres");
        closeServer(socketTCP, -1);
        return -1;
    }
    
    params->operation = strdup(operation);
    params->filename = strdup(filename);
    memcpy(&params->client, client, sizeof(struct sockaddr_in));
    params->socketTCP = socketTCP;
    
    // Créer un thread pour gérer le transfert
    pthread_t thread;
    if (pthread_create(&thread, NULL, fileTransferThreadServer, params) != 0) {
        perror("Échec création thread");
        free(params->operation);
        free(params->filename);
        free(params);
        closeServer(socketTCP, -1);
        return -1;
    }
    
    // Détacher le thread pour qu'il se libère automatiquement à la fin
    pthread_detach(thread);
    
    return 1;  // Succès - traitement en cours dans le thread
}

int main(void) {
    signal(SIGTERM, handleSigint); // dans le cas ou on fait ctrl+c
    signal(SIGINT, handleSigint); // Handler pour Ctrl+C pour le shutdown
    sockfd = initSocket();
    User activeUsers[MAX_USERS];
    int  numActiveUsers = 0;
    char buffer[BUFFER_MAX];
    struct sockaddr_in client;

    while (serverRunning) {
        if (recvMessage(sockfd, buffer, sizeof(buffer), &client) < 0)
            continue;

        MessageInfo msg;
        if (parseMessage(buffer, &msg) != 0) {
            sendAcR(sockfd, &client);
            continue;
        }
        printMessage(&msg);

        // 1) On identifie la commande
        CommandType cmd = getCommandType(msg.payload);
        int         status;
        char        response[BUFFER_MAX];

        // 2) On dispatch via switch
        switch (cmd) {
            case cmdConnect:
                status = connectCmd(msg.payload,
                                    &client,
                                    response,
                                    sizeof(response),
                                    activeUsers,
                                    &numActiveUsers);
                break;

            case cmdDisconnect:
                status = disconnectCmd(msg.payload,
                                       &client,
                                       response,
                                       sizeof(response),
                                       activeUsers,
                                       numActiveUsers);
                break;

            case cmdRegister:
                status = registerUser(msg.payload,
                                      &client,
                                      response,
                                      sizeof(response),
                                      activeUsers,
                                      &numActiveUsers);
                break;

            case cmdMsg: {
                // sendPrivateMsg renvoie l'index du destinataire ou <0
                int destIndex = sendPrivateMsg(msg.payload,
                                               &client,
                                               response,
                                               sizeof(response),
                                               activeUsers,
                                               numActiveUsers);
                if (destIndex >= 0) {
                    // on prépare l'adresse du destinataire
                    struct sockaddr_in dest = {
                        .sin_family = AF_INET,
                        .sin_port   = htons(activeUsers[destIndex].port)
                    };
                    inet_pton(AF_INET,
                              activeUsers[destIndex].ip,
                              &dest.sin_addr);

                    //  a) on envoie le message au destinataire
                    sendto(sockfd,
                           response,
                           strlen(response),
                           0,
                           (struct sockaddr*)&dest,
                           sizeof(dest));

                    //  b) on confirme à l'expéditeur
                    char confirm[BUFFER_MAX];
                    snprintf(confirm,
                             sizeof(confirm),
                             "Message envoyé à %s",
                             activeUsers[destIndex].pseudo);
                    sendto(sockfd,
                           confirm,
                           strlen(confirm),
                           0,
                           (struct sockaddr*)&client,
                           sizeof(client));

                    printf("Message privé envoyé à %s\n",
                           activeUsers[destIndex].pseudo);
                } else {
                    // échec → on renvoie simplement response à l'expéditeur
                    sendto(sockfd,
                           response,
                           strlen(response),
                           0,
                           (struct sockaddr*)&client,
                           sizeof(client));
                }
                // on a tout envoyé pour @msg, donc on passe au prochain tour
                continue;
            }
            
            // @help, @ping, @credits, @shutdown
            case cmdHelp:
                status = helpCmd(msg.payload, 
                                &client, 
                                response, 
                                sizeof(response), 
                                activeUsers, 
                                numActiveUsers);
                break;

            case cmdPing:
                status = pingCmd(msg.payload, 
                                &client, 
                                response, 
                                sizeof(response), 
                                activeUsers, 
                                numActiveUsers);
                break;
            
            case cmdCredits:
                status = creditsCmd(msg.payload,
                                    &client,
                                    response,
                                    sizeof(response),
                                    activeUsers,
                                    numActiveUsers);
                break;

            case cmdShutdown:
                status = shutdownCmd(msg.payload,
                                    &client,
                                    response,
                                    sizeof(response),
                                    activeUsers,
                                    numActiveUsers);
                break;
            
            case cmdUpload:
                // Utiliser directement handleFileTransfer pour les uploads
                status = uploadCmd(msg.payload, 
                                &client, 
                                response, 
                                sizeof(response), 
                                activeUsers, 
                                numActiveUsers);
                if (status == 3) {
                    // Initialiser le transfert de fichier avec threads
                    status = handleFileTransfer(&msg, sockfd, &client);
                }
                break;

            case cmdDownload:
                // Utiliser directement handleFileTransfer pour les downloads
                status = downloadCmd(msg.payload, 
                                &client, 
                                response, 
                                sizeof(response), 
                                activeUsers, 
                                numActiveUsers);
                if (status == 4) {
                    // Initialiser le transfert de fichier avec threads
                    status = handleFileTransfer(&msg, sockfd, &client);
                }
                break;
            
            default: {
                int authenticated = 0;
                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client.sin_addr, client_ip, INET_ADDRSTRLEN);

                for (int i = 0; i < numActiveUsers && !authenticated; i++) {
                    if (activeUsers[i].isConnected
                    && strcmp(activeUsers[i].ip, client_ip) == 0
                    && activeUsers[i].port == ntohs(client.sin_port))
                    {
                        authenticated = 1;
                    }
                }

                if (authenticated) {
                    sendAcR(sockfd, &client);
                } else {
                    const char *auth_req =
                        "Veuillez vous authentifier avec '@connect login mdp'";
                    sendto(sockfd,
                        auth_req,
                        strlen(auth_req),
                        0,
                        (const struct sockaddr*)&client,
                        sizeof(client));
                }
                continue;
            }
        }
        
        // 3) pour toutes les autres commandes, on envoie la réponse ici
        if (status >= 0) {
            sendto(sockfd,
                   response,
                   strlen(response),
                   0,
                   (struct sockaddr*)&client,
                   sizeof(client));
        }

        // Vérifier si c'est une commande d'arrêt
        if (status == 2) {
            printf("Commande d'arrêt reçue d'un administrateur. Arrêt du serveur...\n");
            
            // Notifier tous les utilisateurs connectés
            for (int i = 0; i < numActiveUsers; i++) {
                if (activeUsers[i].isConnected) {
                    struct sockaddr_in dest = {
                        .sin_family = AF_INET,
                        .sin_port = htons(activeUsers[i].port)
                    };
                    inet_pton(AF_INET, activeUsers[i].ip, &dest.sin_addr);
                    
                    const char *shutdownMsg = "Le serveur va s'arrêter. Vous allez être déconnecté.\n";
                    
                    // Envoi avec vérification
                    if (sendto(sockfd, shutdownMsg, strlen(shutdownMsg), 0, 
                            (struct sockaddr*)&dest, sizeof(dest)) < 0) {
                        printf("Erreur lors de l'envoi du message d'arrêt à %s\n", activeUsers[i].pseudo);
                    } else {
                        printf("Message d'arrêt envoyé à %s\n", activeUsers[i].pseudo);
                    }
                }
            }
        
            // Fermer proprement le socket et arrêter le serveur
            close(sockfd);
            raise(SIGINT); // Appeler le handler de signal pour arrêter le serveur
            printf("Serveur arrêté.\n");
        }
    }

    return EXIT_SUCCESS;
}