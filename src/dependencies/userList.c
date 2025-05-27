#include "user.h"
#include "userList.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

userList *createUserList() {
    userList *list = (userList *)malloc(sizeof(userList));
    if (list == NULL) {
        perror("Erreur d'allocation de mémoire pour la liste d'utilisateurs");
        return NULL;
    }

    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    return list;
}
void freeUserList(userList *list) {
    UserNode *current = list->head;
    while (current != NULL) {
        UserNode *next = current->next;
        free(current->user);
        free(current);
        current = next;
    }
    free(list);
}

int addUser(userList *list, char *user) {
    UserNode *newNode = malloc(sizeof(UserNode));
    if (newNode == NULL) {
        perror("Erreur d'allocation de mémoire pour le nouvel utilisateur");
        return -1;
    }
    newNode->user = strdup(user);  // Make a copy of the string
    if (newNode->user == NULL) {
        perror("Erreur d'allocation de mémoire pour le nom d'utilisateur");
        free(newNode);
        return -1;
    }
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


int removeUser(userList *list, const char *pseudo) {
    UserNode *current = list->head;
    UserNode *previous = NULL;

    while (current != NULL) {
        if (strcmp(current->user, pseudo) == 0) {
            if (previous == NULL) {
                list->head = current->next;
            } else {
                previous->next = current->next;
            }
            if (current == list->tail) {
                list->tail = previous;
            }
            free(current->user);
            free(current);
            list->size--;
            return 0;
        }
        previous = current;
        current = current->next;
    }
    return -1; // Utilisateur non trouvé
}
int isUserInList(userList *list, const char *pseudo) {
    UserNode *current = list->head;
    while (current != NULL) {
        if (strcmp(current->user, pseudo) == 0) {
            return 1; // Utilisateur trouvé
        }
        current = current->next;
    }
    return 0; // Utilisateur non trouvé
}
int getUserCount(userList *list) {
    return list->size;
}
