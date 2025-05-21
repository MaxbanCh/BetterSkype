#ifndef SALON_H
#define SALON_H
#include "userList.h"

struct salon_s {
    char *name;
    char *admin;
    userList *users;
};
typedef struct salon_s Salon;

#define SALON_DIR "salon"
#define SALON_NAME_MAX 50

Salon *createSalon(char *name, char *user);

int joinSalon(Salon *salon, char *user);

int leaveSalon(Salon *salon, char *user);

int saveMessage(Salon *salon, char *message, char *username);


//////////////////////////

struct salonNode_s {
    Salon *salon;
    struct salonNode_s *next;
};
typedef struct salonNode_s salonNode;

struct salonList_s {
    struct salonNode_s *head;
    struct salonNode_s *tail;
    int size;
};
typedef struct salonList_s salonList;

salonList *createSalonList();
void freeSalonList(salonList *list);

int addSalon(salonList *list, Salon *salon);
Salon *findSalon(salonList *list, const char *name);
int removeSalon(salonList *list, const char *name);

#endif