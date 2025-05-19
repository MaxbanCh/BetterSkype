#include <stdlib.h>

struct userNode_s {
    User *user;
    struct userNode_s *next;
};
typedef struct userNode_s UserNode;


struct userList_s {
    UserNode *head;
    UserNode *tail;
    int size;
};
typedef struct userList_s userList;

userList *createUserList();
void freeUserList(userList *list);

int addUser(userList *list, User *user);
int removeUser(userList *list, User *user);

int isUserInList(userList *list, const char *pseudo);
int getUserCount(userList *list);