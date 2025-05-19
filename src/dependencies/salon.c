#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "user.h"
#include "salon.h"
#include "message.h"
#include "userList.h"

#define SALON_DIR "salon"

// Fonction de creation d'un salon :
/*
    *var name : nom du salon
    *var user : utilisateur qui a cree le salon

*/
Salon *createSalon(char *name, User *user) {
    // Créer le répertoire du salon
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s.csv", SALON_DIR, name);
    
    // Vérifier si le salon existe déjà
    if (access(filepath, F_OK) != -1) {
        printf("Le salon '%s' existe déjà.\n", name);
        return;
    }
    
    // Créer le fichier CSV
    FILE *file = fopen(filepath, "w");
    if (!file) {
        perror("Erreur lors de la création du fichier salon");
        return;
    }
    
    // Obtenir la date et l'heure actuelles
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char date_str[30];
    strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // Écrire la première ligne indiquant la création
    fprintf(file, "%s,@created@,%s\n", user->pseudo, date_str);
    
    // Fermer le fichier
    fclose(file);
    
    Salon *salon = malloc(sizeof(Salon));
    salon->name = strdup(name);
    salon->admin = user;
    salon->users = createUserList();

    salon->users = addUser(salon->users, user);

    printf("Salon '%s' créé avec succès par %s.\n", name, user->pseudo);
    return salon;
}

int joinSalon(Salon *salon, User *user) {
    char *name = salon->name;
    // Vérifier si le salon existe
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s.csv", SALON_DIR, name);
    
    if (access(filepath, F_OK) == -1) {
        printf("Le salon '%s' n'existe pas.\n", name);
        return -1;
    }
    
    // verifier si l'utilisateur est deja dans le salon
    if (isUserInList(salon->users, user->pseudo)) {
        printf("%s est déjà dans le salon '%s'.\n", user->pseudo, name);
        return -1;
    }

    // Ajouter l'utilisateur au salon
    salon->users = addUser(salon->users, user);
    if (salon->users == NULL) {
        printf("Erreur lors de l'ajout de l'utilisateur au salon.\n");
        return -1;
    }

    FILE *file = fopen(filepath, "a");
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier salon");
        return -1;
    }
    
    // Obtenir la date et l'heure actuelles
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char date_str[30];
    strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // Écrire la ligne d'entrée de l'utilisateur
    fprintf(file, "%s,@joined@,%s\n", user->pseudo, date_str);
    
    // Fermer le fichier
    fclose(file);
    
    printf("%s a rejoint le salon '%s'.\n", user->pseudo, name);
    return 0;
}

int leaveSalon(Salon *salon, User *user) {
    char *name = salon->name;
    // Vérifier si le salon existe
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s.csv", SALON_DIR, name);
    
    if (access(filepath, F_OK) == -1) {
        printf("Le salon '%s' n'existe pas.\n", name);
        return -1;
    }
    
    FILE *file = fopen(filepath, "a");
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier salon");
        return -1;
    }
    
    // Obtenir la date et l'heure actuelles
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char date_str[30];
    strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // Écrire la ligne de sortie de l'utilisateur
    fprintf(file, "%s,@left@,%s\n", user->pseudo, date_str);
    
    // Fermer le fichier
    fclose(file);

    // Supprimer l'utilisateur du salon
    salon->users = removeUser(salon->users, user->pseudo);
    if (salon->users == NULL) {
        printf("Erreur lors de la suppression de l'utilisateur du salon.\n");
        return -1;
    }
    
    printf("%s a quitté le salon '%s'.\n", user->pseudo, name);
    return 0;
}

int broadcastMessage(Salon *salon, MessageInfo *message, char *username, char *response) {
    char *name = salon->name;
    // Vérifier si le salon existe
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s.csv", SALON_DIR, name);
    
    if (access(filepath, F_OK) == -1) {
        printf("Le salon '%s' n'existe pas.\n", name);
        return -1;
    }
    
    FILE *file = fopen(filepath, "a");
    if (!file) {
        perror("Erreur lors de l'ouverture du fichier salon");
        return -1;
    }

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char date_str[30];
    strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // Écrire la ligne de sortie de l'utilisateur
    fprintf(file, "%s,%s,%s\n", username, message->payload, date_str);
    
    // Fermer le fichier
    fclose(file);

    // Envoyer la réponse aux clients
    for (int i = 0; i < salon->users->size; i++) {
        UserNode *current = salon->users->head;
        for (int j = 0; j < i; j++) {
            current = current->next;
        }
        if (current != NULL) {
            // Envoyer le message à l'utilisateur
        }
    }
}




////////////////////////////////////

struct salonNode_s {
    Salon *salon;
    struct salonNode_s *next;
};
typedef struct salonNode_s salonNode;

struct salonList_s {
    struct salonNode_s *head;
    struct salonNode_s *tail;
};
typedef struct salonList_s salonList;

salonList *createSalonList() {
    salonList *list = malloc(sizeof(salonList));
    if (list == NULL) {
        perror("Erreur d'allocation de mémoire pour la liste de salons");
        return NULL;
    }
    list->head = NULL;
    list->tail = NULL;
    return list;
}

void freeSalonList(salonList *list) {
    salonNode *current = list->head;
    while (current != NULL) {
        salonNode *next = current->next;
        free(current->salon);
        free(current);
        current = next;
    }
    free(list);
}

int addSalon(salonList *list, Salon *salon) {
    salonNode *newNode = malloc(sizeof(salonNode));
    if (newNode == NULL) {
        perror("Erreur d'allocation de mémoire pour le nouveau salon");
        return -1;
    }
    newNode->salon = salon;
    newNode->next = NULL;

    if (list->head == NULL) {
        list->head = newNode;
        list->tail = newNode;
    } else {
        list->tail->next = newNode;
        list->tail = newNode;
    }
    return 0;
}

Salon *findSalon(salonList *list, const char *name) {
    salonNode *current = list->head;
    while (current != NULL) {
        if (strcmp(current->salon->name, name) == 0) {
            return current->salon;
        }
        current = current->next;
    }
    return NULL; // Salon non trouvé
}

int removeSalon(salonList *list, const char *name) {
    salonNode *current = list->head;
    salonNode *previous = NULL;

    while (current != NULL) {
        if (strcmp(current->salon->name, name) == 0) {
            if (previous == NULL) {
                list->head = current->next;
            } else {
                previous->next = current->next;
            }
            free(current->salon);
            free(current);
            return 0; // Salon supprimé
        }
        previous = current;
        current = current->next;
    }
    return -1; // Salon non trouvé
}

