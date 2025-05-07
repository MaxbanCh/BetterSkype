#ifndef MESSAGEINFO_H
#define MESSAGEINFO_H

#include <arpa/inet.h>

// si ces constantes ne sont pas définies ailleurs, on les fixe ici :
#ifndef PSEUDO_MAX
#define PSEUDO_MAX 50
#endif

#ifndef BUFFER_MAX
#define BUFFER_MAX 1024
#endif

typedef struct {
    char sender_ip[INET_ADDRSTRLEN];
    char dest_pseudo[PSEUDO_MAX];
    int  part_num;
    int  total_parts;
    char payload[BUFFER_MAX];
} MessageInfo;

#endif