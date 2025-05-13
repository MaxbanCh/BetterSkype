# all:
# 	export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
# 	gcc -c message.c -o message.o
# 	gcc -o libmessage.so -shared message.o

# 	gcc serveur.c -L./ -lmessage -o serveur -Wall -Wl,-rpath,.
# 	gcc client.c -L./ -lmessage -o client -Wall -Wl,-rpath,.

# 	

# Répertoires
SRC_DIR = src
DEP_DIR = $(SRC_DIR)/dependencies
BIN_DIR = bin

# Fichiers sources
CLIENT_SRC = $(SRC_DIR)/client.c
SERVEUR_SRC = $(SRC_DIR)/serveur.c
MESSAGE_SRC = $(DEP_DIR)/message.c
USER_SRC = $(DEP_DIR)/user.c

# Fichiers objets et lib
MESSAGE_OBJ = $(DEP_DIR)/message.o
MESSAGE_LIB = $(DEP_DIR)/libmessage.so
USER_OBJ = $(DEP_DIR)/user.o
USER_LIB = $(DEP_DIR)/libuser.so

# Exécutables
CLIENT_BIN = $(BIN_DIR)/client
SERVEUR_BIN = $(BIN_DIR)/serveur

# Options de compilation
CFLAGS = -Wall -fPIC -I$(DEP_DIR)
LDFLAGS = -L$(DEP_DIR) -lmessage -luser -Wl,-rpath,$(DEP_DIR)

# Cibles
all: $(MESSAGE_LIB) $(USER_LIB) $(CLIENT_BIN) $(SERVEUR_BIN)

$(MESSAGE_LIB): $(MESSAGE_SRC)
	gcc $(CFLAGS) -c $(MESSAGE_SRC) -o $(MESSAGE_OBJ)
	gcc -shared $(MESSAGE_OBJ) -o $(MESSAGE_LIB)
	
$(USER_LIB): $(USER_SRC)
	gcc $(CFLAGS) -c $(USER_SRC) -o $(USER_OBJ)
	gcc -shared $(USER_OBJ) -o $(USER_LIB)

$(CLIENT_BIN): $(CLIENT_SRC)
	gcc $(CFLAGS) $(CLIENT_SRC) -o $(CLIENT_BIN) $(LDFLAGS)

$(SERVEUR_BIN): $(SERVEUR_SRC)
	gcc $(CFLAGS) $(SERVEUR_SRC) -o $(SERVEUR_BIN) $(LDFLAGS)

clean:
	rm -f $(DEP_DIR)/*.o $(DEP_DIR)/libmessage.so $(DEP_DIR)/libuser.so $(BIN_DIR)/client $(BIN_DIR)/serveur

.PHONY: all clean
