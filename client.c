#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int createSocket()
{
	return socket(PF_INET, SOCK_DGRAM, 0);
}

struct sockaddr_in *adServ(int port)
{
	struct sockaddr_in *adServer = malloc(sizeof(struct sockaddr_in));
	adServer->sin_family = AF_INET; 
	adServer->sin_port = htons(port);

	return adServer;
}

int connection(int ds, char *ip, struct sockaddr_in *adServ)
{
	int res = inet_pton(AF_INET, ip, &(adServ->sin_addr));
	socklen_t lgA = sizeof(struct sockaddr_in);

	res = connect(ds, (struct sockaddr *) adServ, lgA);

	return res;
}

void debugConnexion(int res)
{
	if (res == 0)
		printf("ouais c'est connecte\n");
	else
		printf("nope dommage\n");

	return ;
}


ssize_t sendMessage(int dS, struct sockaddr_in *adServer, char *message)
{
	socklen_t lgA = sizeof(struct sockaddr_in);
	return sendto(dS, message, 1024, 0, (struct sockaddr *)adServer, lgA);
}

void debugSendMessage(ssize_t snd)
{
	if (snd == -1)
		perror("Erreur envoi msg : ");

	return ;
}

char *createMessage(char *ip, char *dest, char *msg)
{
	char *message = malloc(1024*sizeof(char));
	
	strcpy(message, ip);
	strcat(message, "//");

	strcat(message, dest);
	strcat(message, "//");

	strcat(message, "1");
	strcat(message, "//");

	strcat(message, "1");
	strcat(message, "/#/#00");

	strcat(message, msg);

	return message;
}

int main()
{
	int dS = createSocket();
	struct sockaddr_in *adServer = adServ(34567);

	int res = connection(dS, "127.0.0.1", adServer);
	debugConnexion(res);

	char messagesend[1024] = "Ouais ouais ouais !";
	char *message = createMessage("127.0.0.1", "nabilb", messagesend);

	ssize_t snd = sendMessage(dS, adServer, message);
	debugSendMessage(snd);
	
	char messagercv[1024];

	recvfrom(dS, messagercv, strlen(messagercv), 0, NULL, NULL);
	printf("reponse : %s \n", messagercv);

	close(dS);

	return 0;
}