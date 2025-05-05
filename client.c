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

// int connect(int port, char *ip)
// {
// 	int dS = createSocket();
// 	struct sockaddr_in *adServer = adServ(port);

// 	int res = connection(dS, port, adServer);

// 	return dS;
// }

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
	printf("size of msg : %d\n", sizeof(message));


	return sendto(dS, message, 256, 0, (struct sockaddr *)adServer, lgA);
}

void debugSendMessage(ssize_t snd)
{
	if (snd == -1)
		perror("Erreur envoi msg : ");

	return ;
}



int main()
{
	int dS = createSocket();
	struct sockaddr_in *adServer = adServ(34567);

	int res = connection(dS, "127.0.0.1", adServer);
	debugConnexion(res);

	char messagesend[256] = "Ouais ouais ouais !";

	ssize_t snd = sendMessage(dS, adServer, messagesend);
	debugSendMessage(snd);
	
	char messagercv[256];

	recvfrom(dS, messagercv, strlen(messagercv), 0, NULL, NULL);
	printf("reponse : %s \n", messagercv);

	close(dS);

	return 0;
}