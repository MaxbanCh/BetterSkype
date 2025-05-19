#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "user.h"
#include "salon.h"

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
    
    // Ajouter l'utilisateur au salon
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
    
    printf("%s a quitté le salon '%s'.\n", user->pseudo, name);
    return 0;
}