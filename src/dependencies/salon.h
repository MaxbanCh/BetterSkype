#include <user.h>
#include "message.h"
#include "userList.h" // Include the header file where userList is defined

struct salon_s {
    char *name;
    User *admin;
    userList *users;
};
typedef struct salon_s Salon;

Salon *createSalon(char *name, User *user);

int joinSalon(Salon *salon, User *user);

int leaveSalon(Salon *salon, User *user);

void listSalons(User *user);