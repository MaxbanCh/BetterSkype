#ifndef CLIENT_H
#define CLIENT_H

int createSocket();
struct sockaddr_in *adServ(int port);
int connection(int ds, char *ip, struct sockaddr_in *adServ);
void debugConnexion(int res);
ssize_t sendMessage(int dS, struct sockaddr_in *adServer, char *message);
void debugSendMessage(ssize_t snd);


#endif