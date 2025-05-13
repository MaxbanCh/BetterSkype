#ifndef COMMANDES_H
#define COMMANDES_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Définition des types de commandes possibles
typedef enum {
    CMD_HELP,       // @help - Affiche la liste des commandes disponibles
    CMD_PING,       // @ping - Le serveur répond "pong"
    CMD_MSG,        // @msg <user> <msg> - Envoie un message privé à un utilisateur 
    CMD_CREDITS,    // @credits - Affiche les crédits de l'application
    CMD_SHUTDOWN,   // @shutdown - Éteint le serveur 
    CMD_UNKNOWN     // Commande non reconnue
    //CMD_DELETE
    //CMD_CONNECT 
    //CMD_DISCONNET
} CommandType;

// Structure d'une commande
typedef struct {
    CommandType type;           // Type de la commande
    char *name;                 // Nom de la commande (ex: "help", "ping", etc.)
    char *description;          // Description de la commande pour l'aide
    int min_args;               // Nombre minimum d'arguments requis
} Command;

// Structure des arguments d'une commande
/*typedef struct {
    CommandType type;           // Type de la commande identifiée
    int argc;                   // Nombre d'arguments
    char **argv;                // Tableau des arguments
    char *raw_message;          // Message brut original
} CommandArgs;*/

// Identifie le type de commande du message 
CommandType getCommandType(const char *message);


// Exécute une commande côté client
int executeClientCommand(CommandType type, const char *message, int socket);


// Exécute une commande côté serveur
int executeServerCommand(CommandType type, const char *message, int socket, void *client_addr, int is_admin);

#endif // COMMANDES_H