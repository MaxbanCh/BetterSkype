# Répertoires
SRC_DIR = src
DEP_DIR = $(SRC_DIR)/dependencies
BIN_DIR = bin

# Fichiers sources
CLIENT_SRC = $(SRC_DIR)/client.c
SERVEUR_SRC = $(SRC_DIR)/serveur.c
MESSAGE_SRC = $(DEP_DIR)/message.c
TCPFILE_SRC = $(DEP_DIR)/TCPFile.c

# Fichiers objets et lib
MESSAGE_OBJ = $(DEP_DIR)/message.o
MESSAGE_LIB = $(DEP_DIR)/libmessage.so
TCPFILE_OBJ = $(DEP_DIR)/TCPFile.o
TCPFILE_LIB = $(DEP_DIR)/libTCPFile.so

# Exécutables
CLIENT_BIN = $(BIN_DIR)/client
SERVEUR_BIN = $(BIN_DIR)/serveur

# Options de compilation
CFLAGS = -Wall -fPIC -I$(DEP_DIR)
LDFLAGS = -L$(DEP_DIR) -lmessage -lTCPFile -Wl,-rpath,$(DEP_DIR)

# Cibles
all: $(MESSAGE_LIB) $(TCPFILE_LIB) $(CLIENT_BIN) $(SERVEUR_BIN)

$(MESSAGE_LIB): $(MESSAGE_SRC)
	gcc $(CFLAGS) -c $(MESSAGE_SRC) -o $(MESSAGE_OBJ)
	gcc -shared $(MESSAGE_OBJ) -o $(MESSAGE_LIB)

$(TCPFILE_LIB): $(TCPFILE_SRC) $(MESSAGE_LIB)
	gcc $(CFLAGS) -c $(TCPFILE_SRC) -o $(TCPFILE_OBJ)
	gcc -shared $(TCPFILE_OBJ) -o $(TCPFILE_LIB)

$(CLIENT_BIN): $(CLIENT_SRC) $(MESSAGE_LIB) $(TCPFILE_LIB)
	gcc $(CFLAGS) $(CLIENT_SRC) -o $(CLIENT_BIN) $(LDFLAGS)

$(SERVEUR_BIN): $(SERVEUR_SRC) $(MESSAGE_LIB) $(TCPFILE_LIB)
	gcc $(CFLAGS) $(SERVEUR_SRC) -o $(SERVEUR_BIN) $(LDFLAGS)

clean:
	rm -f $(DEP_DIR)/*.o $(DEP_DIR)/libmessage.so $(DEP_DIR)/libTCPFile.so $(BIN_DIR)/client $(BIN_DIR)/serveur

.PHONY: all clean