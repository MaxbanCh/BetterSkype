#include <user.h>
#include "message.h"
#include "userList.h" // Include the header file where userList is defined

struct salon_s {
    char *name;
    User *admin;
    userList *users;
};
typedef struct salon_s Salon;

#define SALON_DIR "salon"
#define SALON_NAME_MAX 50

Salon *createSalon(char *name, User *user);

int joinSalon(Salon *salon, User *user);

int leaveSalon(Salon *salon, User *user);

void listSalons(User *user);


//////////////////////////

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

salonList *createSalonList();
void freeSalonList(salonList *list);

int addSalon(salonList *list, Salon *salon);
Salon *findSalon(salonList *list, const char *name);
int removeSalon(salonList *list, const char *name);