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


int serverRunning = 1;

// Handler de signal SIGINT (Ctrl+C) pour arrêter proprement le serveur
static void handleSigint(int sig) {
    (void)sig;  
    printf("\nArrêt serveur\n");
    serverRunning = 0;
    //close(sockfd); 
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
    ssize_t r = recvfrom(sockfd, buffer, buflen - 1, 0, (struct sockaddr*)cli, &len); // réception
    if (r < 0) {
        perror("recvfrom");
        return -1;
    }
    buffer[r] = '\0'; 
    return r;         
}

void printMessage(const MessageInfo *m) {
    printf("Expéditeur IP       : %s\n", m->sender_ip);
    printf("Destinataire pseudo : %s\n", m->dest_pseudo);
    printf("Partie              : %d / %d\n", m->part_num, m->total_parts);
    printf("Contenu             : %s\n\n", m->payload);
}

// Fonction qui envoie un accusé de réception à l’expéditeur du message
void sendAcR(int sockfd, const struct sockaddr_in *cli) {
    const char *acR = "reçu accusé de réception\n"; 
    socklen_t len = sizeof(*cli);
    if (sendto(sockfd, acR, strlen(acR), 0, (const struct sockaddr*)cli, len) < 0) {
        perror("sendto"); 
    }
}

int main(void) {
    signal(SIGINT, handleSigint);
    int  sockfd         = initSocket();
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
            
            // @help, @ping, @credits, @shutdown, @upload, @download
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
                                numActiveUsers);;
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
            /*
            case cmdUpload:
                status = uploadCmd(msg.payload,
                                   &client,
                                   response,
                                   sizeof(response));
                break;

            case cmdDownload:
                status = downloadCmd(msg.payload,
                                     &client,
                                     response,
                                     sizeof(response));
                break;
            */
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
                    
                    const char *shutdownMsg = "Le serveur va s'arrêter....";
                    sendto(sockfd, shutdownMsg, strlen(shutdownMsg), 0, 
                        (struct sockaddr*)&dest, sizeof(dest));
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