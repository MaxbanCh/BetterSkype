#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#include "commandes.h"
#include "message.h"
#include "header.h"

// Tableau de commandes disponibles
static Commande *commandes = NULL;
static int nbCommandes = 0;

// Initialise le tableau des commandes disponibles
void initialiserCommandes() {
    // Nombre de commandes à initialiser
    nbCommandes = 5;
    
    // Allocation du tableau de commandes
    commandes = malloc(nbCommandes * sizeof(Commande));
    if (!commandes) {
        perror("Erreur allocation commandes");
        exit(EXIT_FAILURE);
    }
    
    // Initialisation des commandes
    commandes[0] = (Commande){"help", "Affiche la liste des commandes disponibles", cmd_help, false};
    commandes[1] = (Commande){"ping", "Vérifie si le serveur répond", cmd_ping, false};
    commandes[2] = (Commande){"msg", "Envoie un message privé à un utilisateur (@msg <user> <message>)", cmd_msg, false};
    commandes[3] = (Commande){"credits", "Affiche les crédits de l'application", cmd_credits, false};
    commandes[4] = (Commande){"shutdown", "Éteint correctement le serveur", cmd_shutdown, true};
}

// Libère la mémoire allouée pour les commandes
void libererCommandes() {
    if (commandes) {
        free(commandes);
        commandes = NULL;
    }
}

// Vérifie si le message est une commande
bool estCommande(const char *message) {
    return message[0] == '@';
}

// Traite une commande
bool traiterCommande(int sockfd, struct sockaddr_in *client, const char *message) {
    if (!estCommande(message)) {
        return false;
    }
    
    // Extraire le nom de la commande (sans le @)
    char cmdBuffer[BUFFER_MAX];
    strncpy(cmdBuffer, message + 1, BUFFER_MAX - 1);
    cmdBuffer[BUFFER_MAX - 1] = '\0';
    
    // Trouver l'espace qui sépare le nom de la commande des arguments
    char *args = strchr(cmdBuffer, ' ');
    char *cmdName = cmdBuffer;
    
    // Si des arguments existent, séparer le nom des arguments
    if (args) {
        *args = '\0';  // Termine la chaîne du nom de commande
        args++;        // Pointe sur le début des arguments
    } else {
        args = "";     // Pas d'arguments
    }
    
    // Rechercher la commande dans le tableau
    for (int i = 0; i < nbCommandes; i++) {
        if (strcmp(cmdName, commandes[i].nom) == 0) {
            // Exécuter la fonction associée à la commande
            commandes[i].fonction(sockfd, client, args);
            return true;
        }
    }
    
    // Commande non trouvée
    char reponse[BUFFER_MAX];
    snprintf(reponse, BUFFER_MAX, "Commande inconnue: @%s. Tapez @help pour la liste des commandes.", cmdName);
    envoyerReponse(sockfd, client, reponse);
    return true;
}

// Fonction utilitaire pour envoyer une réponse au client
void envoyerReponse(int sockfd, struct sockaddr_in *client, const char *reponse) {
    char ipstr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client->sin_addr), ipstr, INET_ADDRSTRLEN);
    
    // Créer le message formaté à partir de la réponse
    char *message = createMessage("SERVER", "client", (char *)reponse);
    
    // Envoi de la réponse au client
    socklen_t len = sizeof(*client);
    if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr*)client, len) < 0) {
        perror("sendto dans envoyerReponse");
    }
    
    free(message);
}

// Implémentation des commandes

// Commande help
void cmd_help(int sockfd, struct sockaddr_in *client, const char *args) {
    (void)args; // Arguments non utilisés
    
    char reponse[BUFFER_MAX] = "Liste des commandes disponibles:\n";
    size_t offset = strlen(reponse);
    
    // Ajouter chaque commande et sa description
    for (int i = 0; i < nbCommandes; i++) {
        size_t remaining = BUFFER_MAX - offset;
        int written = snprintf(reponse + offset, remaining, 
                              "@%s - %s\n", 
                              commandes[i].nom, 
                              commandes[i].description);
        
        if (written >= remaining) {
            // Buffer plein, arrêter ici
            break;
        }
        
        offset += written;
    }
    
    envoyerReponse(sockfd, client, reponse);
}

// Commande ping
void cmd_ping(int sockfd, struct sockaddr_in *client, const char *args) {
    (void)args; // Arguments non utilisés
    envoyerReponse(sockfd, client, "pong");
}

// Commande msg pour envoyer un message privé
void cmd_msg(int sockfd, struct sockaddr_in *client, const char *args) {
    if (!args || !*args) {
        envoyerReponse(sockfd, client, "Usage: @msg <utilisateur> <message>");
        return;
    }
    
    // Extraire le destinataire
    char argsBuffer[BUFFER_MAX];
    strncpy(argsBuffer, args, BUFFER_MAX - 1);
    argsBuffer[BUFFER_MAX - 1] = '\0';
    
    char *dest = strtok(argsBuffer, " ");
    if (!dest) {
        envoyerReponse(sockfd, client, "Usage: @msg <utilisateur> <message>");
        return;
    }
    
    // Extraire le message
    char *msg = strtok(NULL, "");
    if (!msg) {
        envoyerReponse(sockfd, client, "Usage: @msg <utilisateur> <message>");
        return;
    }
    
    // TODO: Implémenter la recherche du destinataire et l'envoi du message privé
    // Cette partie dépend de la façon dont vous stockez les utilisateurs connectés
    
    char reponse[BUFFER_MAX];
    snprintf(reponse, BUFFER_MAX, "Message envoyé à %s: %s", dest, msg);
    envoyerReponse(sockfd, client, reponse);
}

// Commande credits
void cmd_credits(int sockfd, struct sockaddr_in *client, const char *args) {
    (void)args; // Arguments non utilisés
    
    // TODO: Plus tard, cette fonction devra lire le contenu du fichier Credits.txt
    const char *credits = "Application de messagerie FAR - 2024-2025\n"
                          "Développée par l'équipe de développement\n"
                          "Version 1.0";
    
    envoyerReponse(sockfd, client, credits);
}

// Commande shutdown
void cmd_shutdown(int sockfd, struct sockaddr_in *client, const char *args) {
    (void)args; // Arguments non utilisés
    
    // TODO: Vérifier si l'utilisateur a les droits d'administration
    
    envoyerReponse(sockfd, client, "Arrêt du serveur en cours...");
    
    // Envoi d'un signal SIGINT au processus serveur pour arrêt propre
    raise(SIGINT);
}