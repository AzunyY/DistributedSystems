/*Grupo que desenvolveu o projeto
*  - Grupo SD - 032
*
*  - 53563 Ana Luís
*  - 56299 Gabriel Gomes Gameiro
*  - 56321 Rodrigo da Silva Antunes
**/

#ifndef _NETWORK_SERVER_PRIVATE_H
#define _NETWORK_SERVER_PRIVATE_H
#include "network_server.h"
#include "inet.h"
#include "message-private.h"
#include "signal.h"
#include "tree-skel-private.h"
#include <poll.h>

#define TIMEOUT -1  // em milisegundos

/*Funcao que inicializa valores de desc_set*/
void setDesc(int i);

/*Trata do ctrlC no servidor */
void ctrlC();

#endif
