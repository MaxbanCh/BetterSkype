#ifndef USERLIST_H
#define USERLIST_H

struct userNode_s {
    char *user;
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

int addUser(userList *list, char *user);
int removeUser(userList *list, char const *user);

int isUserInList(userList *list, const char *pseudo);
int getUserCount(userList *list);

#endif