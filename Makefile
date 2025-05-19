# all:
# 	export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
# 	gcc -c message.c -o message.o
# 	gcc -o libmessage.so -shared message.o
#
# 	gcc serveur.c -L./ -lmessage -o serveur -Wall -Wl,-rpath,.
# 	gcc client.c -L./ -lmessage -o client -Wall -Wl,-rpath,.
#
# 	
# Répertoires
SRC_DIR = src
DEP_DIR = $(SRC_DIR)/dependencies
BIN_DIR = bin

# Fichiers sources
CLIENT_SRC  = $(SRC_DIR)/client.c
SERVEUR_SRC = $(SRC_DIR)/serveur.c
MESSAGE_SRC = $(DEP_DIR)/message.c
USER_SRC    = $(DEP_DIR)/user.c
COMMAND_SRC = $(DEP_DIR)/command.c
TCPFILE_SRC = $(DEP_DIR)/TCPFile.c

# Fichiers objets et lib
MESSAGE_OBJ = $(DEP_DIR)/message.o
MESSAGE_LIB = $(DEP_DIR)/libmessage.so
USER_OBJ    = $(DEP_DIR)/user.o
USER_LIB    = $(DEP_DIR)/libuser.so
COMMAND_OBJ = $(DEP_DIR)/command.o
COMMAND_LIB = $(DEP_DIR)/libcommand.so
TCPFILE_OBJ = $(DEP_DIR)/TCPFile.o
TCPFILE_LIB = $(DEP_DIR)/libTCPFile.so

# Exécutables
CLIENT_BIN  = $(BIN_DIR)/client
SERVEUR_BIN = $(BIN_DIR)/serveur

# Options de compilation
CFLAGS  = -Wall -fPIC -I$(DEP_DIR)
LDFLAGS = -L$(DEP_DIR) -lcommand -luser -lmessage -lTCPFile -Wl,-rpath,$(DEP_DIR) -pthread

# Cibles
all: $(MESSAGE_LIB) $(USER_LIB) $(COMMAND_LIB) $(TCPFILE_LIB) $(CLIENT_BIN) $(SERVEUR_BIN)

$(MESSAGE_LIB): $(MESSAGE_SRC)
	gcc $(CFLAGS) -c $(MESSAGE_SRC) -o $(MESSAGE_OBJ)
	gcc -shared $(MESSAGE_OBJ) -o $(MESSAGE_LIB)
		
$(USER_LIB): $(USER_SRC)
	gcc $(CFLAGS) -c $(USER_SRC) -o $(USER_OBJ)
	gcc -shared $(USER_OBJ) -o $(USER_LIB)

$(COMMAND_LIB): $(COMMAND_SRC) $(DEP_DIR)/header.h $(DEP_DIR)/command.h $(USER_LIB) $(MESSAGE_LIB)
	gcc $(CFLAGS) -c $(COMMAND_SRC) -o $(COMMAND_OBJ)
	gcc -shared $(COMMAND_OBJ) -o $(COMMAND_LIB) \
	    -L$(DEP_DIR) -luser -lmessage -Wl,-rpath,$(DEP_DIR)

$(TCPFILE_LIB): $(TCPFILE_SRC) $(MESSAGE_LIB)
	gcc $(CFLAGS) -c $(TCPFILE_SRC) -o $(TCPFILE_OBJ)
	gcc -shared $(TCPFILE_OBJ) -o $(TCPFILE_LIB)

$(CLIENT_BIN): $(CLIENT_SRC) $(MESSAGE_LIB) $(TCPFILE_LIB)
	gcc $(CFLAGS) $(CLIENT_SRC) -o $(CLIENT_BIN) $(LDFLAGS) -pthread

$(SERVEUR_BIN): $(SERVEUR_SRC) $(MESSAGE_LIB) $(TCPFILE_LIB)
	gcc $(CFLAGS) $(SERVEUR_SRC) -o $(SERVEUR_BIN) $(LDFLAGS) -pthread

clean:
	rm -f $(DEP_DIR)/*.o $(DEP_DIR)/libmessage.so $(DEP_DIR)/libuser.so $(DEP_DIR)/libcommand.so $(DEP_DIR)/libTCPFile.so $(BIN_DIR)/client $(BIN_DIR)/serveur

.PHONY: all clean