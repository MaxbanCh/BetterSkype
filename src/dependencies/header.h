#ifndef HEADER_H
#define HEADER_H

// Port d'écoute du serveur
#define PORT 12345

// Taille maximale du buffer de réception
#define BUFFER_MAX 1024

// Délimiteur des champs dans la structure du message
#define FIELD_DELIM "//"

// Délimiteur entre les champs et le contenu
#define PAYLOAD_DELIM "/#/#00"


#define USERS_FILE "users.csv"

#define MAX_USERS 100

#endif // HEADER_H