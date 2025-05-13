#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "commandes.h"
#include "message.h"
#include "header.h"

// Tableau des commandes disponibles
const Command AVAILABLE_COMMANDS[] = {
    {CMD_HELP, "help", "Affiche la liste des commandes disponibles", 0},
    {CMD_PING, "ping", "Le serveur répond \"pong\"", 0},
    {CMD_MSG, "msg", "Envoie un message privé à un utilisateur (@msg <user> <message>)", 2},
    {CMD_CREDITS, "credits", "Affiche les crédits de l'application", 0},
    {CMD_SHUTDOWN, "shutdown", "Éteint le serveur", 0}
    // CMD_DELETE
    // CMD_CONNECT 
    // CMD_DISCONNECT
};

// Nombre de commandes dans le tableau AVAILABLE_COMMANDS (utilisé pour parcourir les commandes)
const int NUM_COMMANDS = sizeof(AVAILABLE_COMMANDS) / sizeof(Command);

// Détermine si un message est une commande (commence par @)
int isCommand(const char *message) {
    if (!message || strlen(message) < 1) {
        return 0; // Message vide 
    }
    return message[0] == '@'; // TRUE => commande, FALSE => message simple
}

// Identifie le type de commande du message 
CommandType getCommandType(const char *message) {
    CommandType result = CMD_UNKNOWN;
    char *cmd_name = NULL;
    int found = 0;
    
    if (isCommand(message)) {
        // Extraire le nom de la commande (sans le @)
        cmd_name = strdup(message + 1);
        
        // Vérifier que l'allocation s'est bien passée
        if (cmd_name != NULL) {
            // Identifier le premier espace pour isoler le nom de la commande des arguments potentiels
            char *space = strchr(cmd_name, ' ');
            if (space != NULL) {
                *space = '\0';  // Terminer la chaîne au premier espace
            }
            
            // Recherche de la commande dans le tableau de commandes disponibles
            for (int i = 0; i < NUM_COMMANDS && result == CMD_UNKNOWN; i++) {
                if (strcmp(cmd_name, AVAILABLE_COMMANDS[i].name) == 0) {
                    result = AVAILABLE_COMMANDS[i].type;
                }
            }
            
            free(cmd_name);
        }
    }
    
    return result;
}



