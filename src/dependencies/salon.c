#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "salon.h"

#define SALON_DIR "salon"

// Fonction de creation d'un salon :
/*
    *var name : nom du salon
    *var user : utilisateur qui a cree le salon

*/
Salon *createSalon(char *name, char *user) {
    Salon *salon = (Salon *)malloc(sizeof(Salon));
    salon->name = strdup(name);
    salon->admin = user;
    salon->users = createUserList();

    int res = addUser(salon->users, user);
    if (res != 0)
    {
        printf("pb");
    }

    printf("Salon '%s' créé avec succès par %s.\n", name, user);

    // Créer le répertoire du salon
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s.csv", SALON_DIR, name);
    
    // Vérifier si le salon existe déjà
    if (access(filepath, F_OK) != -1) {
        printf("Le fichier salon '%s' existe déjà.\n", name);

        return salon;
    }
    else {
        
        // Créer le fichier CSV
        FILE *file = fopen(filepath, "w");
        if (!file) {
            perror("Erreur lors de la création du fichier salon");
            return salon;
        }
        
        // Obtenir la date et l'heure actuelles
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char date_str[30];
        strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M:%S", tm_info);
        
        // Écrire la première ligne indiquant la création
        fprintf(file, "%s,@created@,%s\n", user, date_str);
        
        // Fermer le fichier
        fclose(file);

        return salon;
    }
}

int joinSalon(Salon *salon, char *user) {
    char *name = salon->name;
    // Vérifier si le salon existe
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s.csv", SALON_DIR, name);
    
    if (access(filepath, F_OK) == -1) {
        printf("Le salon '%s' n'existe pas.\n", name);
        return -1;
    }
    
    printf("Le salon '%s' existe.\n", name);
    // verifier si l'utilisateur est deja dans le salon
    if (isUserInList(salon->users, user)) {
        printf("%s est déjà dans le salon '%s'.\n", user, name);
        return -1;
    }

    printf("%s n'est pas dans le salon '%s'.\n", user, name);
    // Ajouter l'utilisateur au salon
    int res = addUser(salon->users, user);
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
    fprintf(file, "%s,@joined@,%s\n", user, date_str);
    
    // Fermer le fichier
    fclose(file);
    
    printf("%s a rejoint le salon '%s'.\n", user, name);
    return 0;
}

int leaveSalon(Salon *salon, char *user) {
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
    fprintf(file, "%s,@left@,%s\n", user, date_str);
    
    // Fermer le fichier
    fclose(file);

    // Supprimer l'utilisateur du salon
    salon->users = removeUser(salon->users, user);
    if (salon->users == NULL) {
        printf("Erreur lors de la suppression de l'utilisateur du salon.\n");
        return -1;
    }
    
    printf("%s a quitté le salon '%s'.\n", user, name);
    return 0;
}

int broadcastMessage(Salon *salon, char *message, char *username, char *response, userList *users) {
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
    fprintf(file, "%s,%s,%s\n", username, message, date_str);
    
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

salonList *createSalonList() {
    salonList *list = malloc(sizeof(salonList));
    if (list == NULL) {
        perror("Erreur d'allocation de mémoire pour la liste de salons");
        return NULL;
    }
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
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
    list->size++;
    return 0;
}

Salon *findSalon(salonList *list, const char *name) {
    salonNode *current = list->head;
    while (current != NULL) {
        // Comparer le nom du salon avec le nom recherché
        printf("Comparaison de %s avec %s\n", current->salon->name, name);
        if (strcmp(current->salon->name, name) == 0) {
            printf("Salon trouvé: %s\n", current->salon->name);
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
            list->size--;
            if (list->size == 0) {
                list->tail = NULL;
            }
            return 0; // Salon supprimé
        }
        previous = current;
        current = current->next;
    }
    return -1; // Salon non trouvé
}
