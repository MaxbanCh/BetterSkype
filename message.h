#include <arpa/inet.h>

struct message{
    char *exp[INET_ADDRSTRLEN]; // adresse IP de l'expéditeur
    char *pseudoDest[16]; // adresse IP du destinataire
    int numeroDeMessage; // numéro de la partie du message
    int nbMessagesTotal; // nombre total de parties du message
    char *message[256]; // message
} typedef msg;

