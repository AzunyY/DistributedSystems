/*Grupo que desenvolveu o projeto
*  - Grupo SD - 032
*
*  - 53563 Ana Luís
*  - 56299 Gabriel Gomes Gameiro
*  - 56321 Rodrigo da Silva Antunes
**/

#ifndef _TREE_SKEL_PRIVATE_H
#define _TREE_SKEL_PRIVATE_H

#include "tree_skel.h"
#include "message-private.h"
#include "network_server-private.h"
#include "inet.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include "zookeeper/zookeeper.h"

struct op_proc{
  int max_proc;
  int* in_progress;
};

struct request_t {
int op_n; 	//o número da operação
int op; 	//a operação a executar. op=0 se for um delete, op=1 se for um put
char* key; 	//a chave a remover ou adicionar
struct data_t* data; 	// os dados a adicionar em caso de put, ou NULL em caso de delete
struct request_t* next;
};

/*Funcao que liberta memoria*/
void destroy_request(struct request_t *request);

/*Funcao auxiliar para adicionar request a queue*/
int add_request_to_queue_aux(struct request_t* request, MessageT *msg);

/* Funçao que adiciona uma request ah lista de requests e retorna o numero do request*/
int add_request_to_queue(MessageT *msg);

/*Funcao de controlo de entrada*/
void enterMain();

/*Funcao de controlo de saida*/
void leaveMain();

/*Funcao de controlo de entrada*/
void enterSecondaryThread();

/*Cria uma estrutura de pedidio*/
struct request_t* startQueue(struct request_t* queue);

/*FUncao de controlo de saida*/
void leaveSecondaryThread();

/*Funcao que atualiza o pointer e o queue_head*/
void queueuPointerUpdate();

/*Funcao que faz as operacoes de put e del*/
int treeUpdate(int op_result);

/*Funcao que atualiza a estrutura op_proc*/
void procUpdate(int op_result);

int tree_init_zoo_server(char* zoo_host, char* port, char*zoo_ip, char* zoo_port);

void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx);

void swatcher_func(zhandle_t *zzh, int type, int state, const char *path, void *watcherCtx);

#endif
