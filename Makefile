# Répertoires
SRC_DIR = src
DEP_DIR = $(SRC_DIR)/dependencies
BIN_DIR = bin

# Fichiers sources
CLIENT_SRC  = $(SRC_DIR)/client.c
SERVEUR_SRC = $(SRC_DIR)/serveur.c
MESSAGE_SRC = $(DEP_DIR)/message.c
USER_SRC	= $(DEP_DIR)/user.c
COMMAND_SRC = $(DEP_DIR)/command.c
TCPFILE_SRC = $(DEP_DIR)/TCPFile.c
USERLIST_SRC = $(DEP_DIR)/userList.c
SALON_SRC = $(DEP_DIR)/salon.c

# Fichiers objets et lib
MESSAGE_OBJ = $(DEP_DIR)/message.o
MESSAGE_LIB = $(DEP_DIR)/libmessage.so
USER_OBJ	= $(DEP_DIR)/user.o
USER_LIB	= $(DEP_DIR)/libuser.so
COMMAND_OBJ = $(DEP_DIR)/command.o
COMMAND_LIB = $(DEP_DIR)/libcommand.so
TCPFILE_OBJ = $(DEP_DIR)/TCPFile.o
TCPFILE_LIB = $(DEP_DIR)/libTCPFile.so
USERLIST_OBJ = $(DEP_DIR)/userList.o
USERLIST_LIB = $(DEP_DIR)/libuserList.so
SALON_OBJ = $(DEP_DIR)/salon.o
SALON_LIB = $(DEP_DIR)/libsalon.so

# Exécutables
CLIENT_BIN  = $(BIN_DIR)/client
SERVEUR_BIN = $(BIN_DIR)/serveur

# Options de compilation
CFLAGS  = -Wall -fPIC -I$(DEP_DIR)	
LDFLAGS = -L$(DEP_DIR) -lcommand -luser -lmessage -lTCPFile -lsalon -luserList -Wl,-rpath,$(DEP_DIR) -pthread

# Cibles
all: $(BIN_DIR) $(MESSAGE_LIB) $(USERLIST_LIB) $(USER_LIB) $(SALON_LIB) $(COMMAND_LIB) $(TCPFILE_LIB) $(CLIENT_BIN) $(SERVEUR_BIN)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(MESSAGE_LIB): $(MESSAGE_SRC)
	gcc $(CFLAGS) -c $(MESSAGE_SRC) -o $(MESSAGE_OBJ)
	gcc -shared $(MESSAGE_OBJ) -o $(MESSAGE_LIB)

$(USERLIST_LIB): $(USERLIST_SRC)
	gcc $(CFLAGS) -c $(USERLIST_SRC) -o $(USERLIST_OBJ)
	gcc -shared $(USERLIST_OBJ) -o $(USERLIST_LIB)
		
$(USER_LIB): $(USER_SRC) $(MESSAGE_LIB)
	gcc $(CFLAGS) -c $(USER_SRC) -o $(USER_OBJ)
	gcc -shared $(USER_OBJ) -o $(USER_LIB) \
		-L$(DEP_DIR) -lmessage -Wl,-rpath,$(DEP_DIR)

$(SALON_LIB): $(SALON_SRC) $(USERLIST_LIB)
	gcc $(CFLAGS) -c $(SALON_SRC) -o $(SALON_OBJ)
	gcc -shared $(SALON_OBJ) -o $(SALON_LIB) \
		-L$(DEP_DIR) -luserList -Wl,-rpath,$(DEP_DIR)

$(COMMAND_LIB): $(COMMAND_SRC) $(DEP_DIR)/header.h $(DEP_DIR)/command.h $(USER_LIB) $(MESSAGE_LIB) $(USERLIST_LIB) $(SALON_LIB)
	gcc $(CFLAGS) -c $(COMMAND_SRC) -o $(COMMAND_OBJ)
	gcc -shared $(COMMAND_OBJ) -o $(COMMAND_LIB) \
		-L$(DEP_DIR) -luser -lmessage -lsalon -luserList -Wl,-rpath,$(DEP_DIR)

$(TCPFILE_LIB): $(TCPFILE_SRC) $(MESSAGE_LIB)
	gcc $(CFLAGS) -c $(TCPFILE_SRC) -o $(TCPFILE_OBJ)
	gcc -shared $(TCPFILE_OBJ) -o $(TCPFILE_LIB) \
		-L$(DEP_DIR) -lmessage -Wl,-rpath,$(DEP_DIR)

$(CLIENT_BIN): $(CLIENT_SRC) $(MESSAGE_LIB) $(TCPFILE_LIB) $(COMMAND_LIB) $(SALON_LIB) $(USERLIST_LIB)
	gcc $(CFLAGS) $(CLIENT_SRC) -o $(CLIENT_BIN) $(LDFLAGS) -pthread

$(SERVEUR_BIN): $(SERVEUR_SRC) $(MESSAGE_LIB) $(TCPFILE_LIB) $(COMMAND_LIB) $(SALON_LIB) $(USERLIST_LIB)
	gcc $(CFLAGS) $(SERVEUR_SRC) -o $(SERVEUR_BIN) $(LDFLAGS) -pthread

clean:
	rm -f $(DEP_DIR)/*.o $(DEP_DIR)/lib*.so $(BIN_DIR)/client $(BIN_DIR)/serveur

.PHONY: all clean