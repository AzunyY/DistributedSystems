# Grupo que desenvolveu o projeto
#  - Grupo SD - 032
#
#  - 53563 Ana Luís
#  - 56299 Gabriel Gomes Gameiro
#  - 56321 Rodrigo da Silva Antunes

client-lib_OBJ_DEP = network_client.o client_stub.o entry.o data.o message.o sdmessage.pb-c.o
tree-client_OBJ_DEP = tree_client.o client-lib.o
tree-server_OBJ_DEP = tree_server.o network_server.o tree_skel.o tree.o entry.o data.o message.o sdmessage.pb-c.o client_stub.o network_client.o

BIN_DIR = binary
SRC_DIR = source
INC_DIR = include
LIB_DIR = lib
PROTO_INC__LIB_DIR = -lzookeeper_mt -I/usr/local/include -L/usr/local/lib -lprotobuf-c
OBJ_DIR = object
TEST_DIR = source

CFLAGS = -Wall -lzookeeper -lhashtable -pthread -lm -g -I $(INC_DIR)
CC = gcc

vpath %.o $(OBJ_DIR)

all: client-lib.o tree-client tree-server

client-lib.o: $(client-lib_OBJ_DEP)
	ld -r $(addprefix $(OBJ_DIR)/,$(client-lib_OBJ_DEP)) -o $(LIB_DIR)/client-lib.o

tree-client: $(tree-client_OBJ_DEP)
	$(CC) $(OBJ_DIR)/tree_client.o $(LIB_DIR)/client-lib.o $(PROTO_INC__LIB_DIR) -o $(BIN_DIR)/tree-client

tree-server: $(tree-server_OBJ_DEP)
	$(CC) $(addprefix $(OBJ_DIR)/,$(tree-server_OBJ_DEP)) $(PROTO_INC__LIB_DIR) -pthread -o $(BIN_DIR)/tree-server

sdmessage.pb-c.o: proto

proto:
	protoc --c_out=. sdmessage.proto
	mv sdmessage.pb-c.h $(INC_DIR)/
	mv sdmessage.pb-c.c $(SRC_DIR)/

%.o: $(SRC_DIR)/%.c $($@)
	$(CC) -c $< $(CFLAGS) -o $(OBJ_DIR)/$@

clean:

	rm -f $(BIN_DIR)/*
	rm -f $(LIB_DIR)/client-lib.o
	rm -f $(OBJ_DIR)/*.o
