/*Grupo que desenvolveu o projeto
*  - Grupo SD - 032
*
*  - 53563 Ana Luís
*  - 56299 Gabriel Gomes Gameiro
*  - 56321 Rodrigo da Silva Antunes
**/

#ifndef _CLIENT_STUB_PRIVATE_H
#define _CLIENT_STUB_PRIVATE_H

#include "message-private.h"
#include "inet.h"
#include "network_client.h"
#include "client_stub.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include "zookeeper/zookeeper.h"

//estrutura da arvore
struct rtree_t {

  int socket;
	struct sockaddr_in server;
  char* ip;

};

#endif
