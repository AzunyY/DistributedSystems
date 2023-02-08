/*Grupo que desenvolveu o projeto
*  - Grupo SD - 032
*
*  - 53563 Ana Luís
*  - 56299 Gabriel Gomes Gameiro
*  - 56321 Rodrigo da Silva Antunes
**/

#include "tree-skel-private.h"
#include "zookeeper/zookeeper.h"
#include "client_stub-private.h"

#define ZDATALEN 1024 * 1024
static int zoo_data_len = ZDATALEN;
static int is_connected; // 1 se o servidor esta ligado ao zookeeper, c.c 0
static int last_assigned;
static int counter; //Variavel de estado
extern int sair; //Variavel para sair do ctrlC

static char *my_id;
static char *watcher_ctx = "ZooKeeper Data Watcher";
static char *root_path = "/chain"; // path do node normal

typedef struct String_vector zoo_string;
static zhandle_t *zh;
struct rtree_t *next_server, *my_server;
struct tree_t *tree;
struct op_proc *proc;
struct request_t *queue_head, *pointer, *sending;

pthread_mutex_t queue_lock, tree_lock, proc_lock, queue_sending =  PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_not_empty, not_main = PTHREAD_COND_INITIALIZER;
pthread_t process;

/* Inicia o skeleton da árvore.
* O main() do servidor deve chamar esta função antes de poder usar a
* função invoke().
* A função deve lançar N threads secundárias responsáveis por atender
* pedidos de escrita na árvore.
* Retorna 0 (OK) ou -1 (erro, por exemplo OUT OF MEMORY)
*/
int tree_skel_init() {
  tree = tree_create();

  if (tree == NULL)
    return -1;

  last_assigned = 1;
  counter = 1;
  sending = startQueue(sending);
  proc = (struct op_proc*) malloc(sizeof(struct op_proc));

  if(proc == NULL){
    printErrorMessage("Malloc error");
    return -1;
  }

  proc -> max_proc = 0;
  proc -> in_progress = calloc(1, sizeof(int));

  if(proc -> in_progress == NULL){
    printErrorMessage("Malloc error");
    return -1;
  }

  if (pthread_create(&process, NULL, &process_request, NULL) != 0) {
    perror("\nError creating thread.\n");
    exit(EXIT_FAILURE);
  }

  return 0;
}

/* Liberta toda a memória e recursos alocados pela função tree_skel_init.
 */
void tree_skel_destroy() {

  pthread_cond_broadcast(&queue_not_empty);
  pthread_join(process, NULL);

  if(queue_head != NULL){
    data_destroy(queue_head -> data);
    free(queue_head -> key);
    while(queue_head -> next != NULL){
      data_destroy(queue_head -> next -> data);
      free(queue_head -> next -> key);
    }
    free(queue_head);
  }

  if(sending != NULL){
    if(sending -> key != NULL)
      free(sending -> key);
    if(sending -> data != NULL)
      data_destroy(sending -> data);
    free(sending);
  }

  if(pointer != NULL){
    data_destroy(pointer -> data);
    free(pointer -> key);
    free(pointer);
  }

  free(proc -> in_progress);
  free(proc);
  free(my_id);

  if(next_server != NULL)
    rtree_disconnect(next_server);
  if(my_server != NULL)
    rtree_disconnect(my_server);
  zookeeper_close(zh);
  tree_destroy(tree);
}

/* Executa uma operação na árvore (indicada pelo opcode contido em msg)
 * e utiliza a mesma estrutura message_t para devolver o resultado.
 * Retorna 0 (OK) ou -1 (erro, por exemplo, árvore nao incializada)
*/
int invoke(MessageT *msg) {

  if (msg == NULL)
    return -1;

  if (tree == NULL) {
    msg -> c_type = MESSAGE_T__C_TYPE__CT_BAD;
    msg -> opcode = MESSAGE_T__OPCODE__OP_ERROR;
    return -1;
  }

  int return_value = 0;
  switch (msg -> opcode) {

    case MESSAGE_T__OPCODE__OP_SIZE:
      msg -> opcode = msg -> opcode + 1;
      msg -> c_type = MESSAGE_T__C_TYPE__CT_RESULT;

      pthread_mutex_lock(&tree_lock);
      msg -> result = tree_size(tree);
      pthread_mutex_unlock(&tree_lock);
      break;

    case MESSAGE_T__OPCODE__OP_GET:
      msg -> opcode = msg -> opcode + 1;
      msg -> c_type = MESSAGE_T__C_TYPE__CT_VALUE;

      struct data_t *data;
      pthread_mutex_lock(&tree_lock);
      data = tree_get(tree, msg -> key);
      pthread_mutex_unlock(&tree_lock);

      if (data == NULL) {
        msg -> data = NULL;
        msg -> datasize = 0;
      }

      else {
        msg -> datasize = data -> datasize;
        msg -> data = strdup(data -> data);
      }

      data_destroy(data);
      break;

    case MESSAGE_T__OPCODE__OP_DEL:
      msg -> opcode = msg -> opcode + 1;
      msg -> c_type = MESSAGE_T__C_TYPE__CT_RESULT;

      msg -> result = add_request_to_queue(msg);

      if (msg -> result == -1) {
        msg -> opcode = MESSAGE_T__OPCODE__OP_ERROR;
        msg -> c_type = MESSAGE_T__C_TYPE__CT_NONE;
        return_value = -1;
      }
      break;

    case MESSAGE_T__OPCODE__OP_PUT:
      msg -> opcode = msg -> opcode + 1;
      msg -> c_type = MESSAGE_T__C_TYPE__CT_RESULT;

      msg -> result = add_request_to_queue(msg);

      if (msg -> result == -1) {
        msg -> opcode = MESSAGE_T__OPCODE__OP_ERROR;
        msg -> c_type = MESSAGE_T__C_TYPE__CT_NONE;
        return_value = -1;
      }
      break;

    case MESSAGE_T__OPCODE__OP_GETKEYS:
        msg -> opcode = msg -> opcode + 1;
        msg -> c_type = MESSAGE_T__C_TYPE__CT_KEYS;

        pthread_mutex_lock(&tree_lock);
        msg -> keys = tree_get_keys(tree);
        msg -> n_keys = tree_size(tree);
        pthread_mutex_unlock(&tree_lock);
        break;

    case MESSAGE_T__OPCODE__OP_GETVALUES:
        msg -> opcode = msg -> opcode + 1;
        msg -> c_type = MESSAGE_T__C_TYPE__CT_VALUES;

        pthread_mutex_lock(&tree_lock);
        msg -> n_values = tree_size(tree);
        msg -> values = (char**) tree_get_values(tree);
        pthread_mutex_unlock(&tree_lock);
        break;

      case MESSAGE_T__OPCODE__OP_HEIGHT:
        msg -> opcode = msg -> opcode + 1;
        msg -> c_type = MESSAGE_T__C_TYPE__CT_RESULT;

        pthread_mutex_lock(&tree_lock);
        msg -> result = tree_height(tree);
        pthread_mutex_unlock(&tree_lock);
        break;

      case MESSAGE_T__OPCODE__OP_VERIFY:

        msg -> opcode = msg -> opcode + 1;
        msg -> c_type = MESSAGE_T__C_TYPE__CT_RESULT;
        msg -> result = verify(msg -> result);

        if (msg -> result == -1) {
          msg -> opcode = MESSAGE_T__OPCODE__OP_ERROR;
          msg -> c_type = MESSAGE_T__C_TYPE__CT_NONE;
          return_value = -1;
        }
      break;

      default:
        msg -> opcode = MESSAGE_T__OPCODE__OP_ERROR;
        msg -> c_type = MESSAGE_T__C_TYPE__CT_BAD;
        return_value = -1;
    }
    return return_value;
}

/*Funcao de controlo de entrada*/
void enterMain(){
  pthread_mutex_lock(&proc_lock);
  while(!sair && counter < 1)
    pthread_cond_wait(&not_main, &proc_lock);
  counter = 0;
  pthread_mutex_unlock(&proc_lock);
}

/*Funcao de controlo de saida*/
void leaveMain(){
  pthread_mutex_lock(&proc_lock);
  counter = 1;
  pthread_cond_broadcast(&not_main);
  pthread_mutex_unlock(&proc_lock);
}

/* Verifica se a operação identificada por op_n foi executada.*/
int verify(int op_n) {
  enterMain();
  if(op_n >= proc -> max_proc){
    if(op_n == proc -> in_progress[0])
      return 0;
  }

  leaveMain();
  return op_n < last_assigned ? 1 : -1;
}

/*Funcao de controlo de entrada*/
void enterSecondaryThread(){
  pthread_mutex_lock(&proc_lock);
  while(!sair && counter == 0)
    pthread_cond_wait(&not_main, &proc_lock);
  counter --;
  pthread_mutex_unlock(&proc_lock);
}

/*FUncao de controlo de saida*/
void leaveSecondaryThread(){
  pthread_mutex_lock(&proc_lock);
  counter++;
  pthread_cond_broadcast(&not_main);
  pthread_mutex_unlock(&proc_lock);
}

/*Funcao que atualiza o pointer e o queue_head*/
void queueuPointerUpdate(){
  //Faz free do request anterior ao queue_head atual
  if (pointer != NULL){
    free(pointer -> key);
    data_destroy(pointer -> data);
    free(pointer);
  }

  //Atualiza o pointer para apontar para o queue_head corrente
  pointer = queue_head;
  //queue_head vai ficar a apontar para o proximo pedido
  queue_head = queue_head -> next;
}

/*Funcao que faz as operacoes de put e del*/
int treeUpdate(int op_result){
  if (pointer -> op == 0)
    op_result = tree_del(tree, pointer -> key);
  else if (pointer -> op == 1)
    op_result = tree_put(tree, pointer -> key, pointer -> data);
  return op_result;
}

/*Funcao que atualiza a estrutura op_proc*/
void procUpdate(int op_result){
  proc -> in_progress[0] = 0;
  if (op_result != -1)
    proc -> max_proc = proc -> max_proc < last_assigned ? last_assigned : proc -> max_proc;
}

void replicateChain(){

  if (next_server != NULL) {
    if (pointer -> op == 1){
      if(sending != NULL){
        if(sending -> data != NULL){
          data_destroy(sending -> data);
        }
        if(sending -> key != NULL)
          free(sending -> key);
      }

      sending -> data = data_dup(pointer -> data);
      sending -> key = strdup(pointer -> key);
      sending -> op = pointer -> op;
      sending -> op_n = pointer -> op_n;
      struct entry_t *entry = entry_create(pointer -> key, pointer -> data);

      pthread_mutex_lock(&queue_sending);
      rtree_put(next_server, entry);
      sleep(2);
      int check = rtree_verify(next_server, sending -> op_n);

      while (check != 1){
        rtree_put(next_server, entry);
        sleep(2);
        check = rtree_verify(next_server, sending -> op_n);
      }
      pthread_mutex_unlock(&queue_sending);

      free(entry);
    }
    else if (pointer -> op == 0){

      if(sending != NULL){
        if(sending -> key != NULL)
          free(sending -> key);
      }

      sending -> key = strdup(pointer -> key);
      sending -> op = pointer -> op;
      sending -> op_n = pointer -> op_n;

      pthread_mutex_lock(&queue_sending);
      rtree_del(next_server, sending -> key);
      sleep(2);
      int check = rtree_verify(next_server, sending -> op_n);

      while (check != 1){
        rtree_del(next_server, sending -> key);
        sleep(2);
        check = rtree_verify(next_server, sending -> op_n);
      }
      pthread_mutex_unlock(&queue_sending);
    }
  }
  return;
}

/*
* Função da thread secundária que vai processar pedidos de escrita.
*/
void *process_request(void* params) {

  while (!sair) {

    pthread_mutex_lock(&queue_lock);
    while (queue_head == NULL && !sair)
      pthread_cond_wait(&queue_not_empty, &queue_lock); /* Espera haver algo na queue*/

    if(sair){
      pthread_mutex_unlock(&queue_lock); /*Desbloqueia se houver ctrlC*/
      return NULL;
    }
    pthread_mutex_unlock(&queue_lock);

    enterSecondaryThread();
    int op_result = -1;
    proc -> in_progress[0] = queue_head -> op_n;
    leaveSecondaryThread();

    pthread_mutex_lock(&queue_lock);
    queueuPointerUpdate();
    pthread_mutex_unlock(&queue_lock);

    enterSecondaryThread();
    procUpdate(op_result);
    leaveSecondaryThread();

    pthread_mutex_lock(&tree_lock);
    op_result = treeUpdate(op_result);
    pthread_mutex_unlock(&tree_lock);

    replicateChain();
  }
  return NULL;
}

/*Funcao auxiliar para adicionar request a queue*/
int add_request_to_queue_aux(struct request_t* request, MessageT *msg){

  if(msg -> opcode == MESSAGE_T__OPCODE__OP_PUT + 1)
    request -> op = 1;
  else if(msg -> opcode == MESSAGE_T__OPCODE__OP_DEL + 1)
    request -> op = 0;

  if (request -> op == 0 && msg -> key == NULL)
    return -1;

  if (request -> op == 1 && (msg -> key == NULL || msg -> data == NULL))
    return -1;

  if(msg -> key != NULL)
    request -> key = strdup(msg -> key);
  if(msg -> data != NULL){
    struct data_t* data = data_create2(msg -> datasize, msg -> data);
    request -> data = data_dup(data);
    free(data);
  }

  request -> op_n = last_assigned;

  last_assigned += 1;
  return request -> op_n;
}

/*Cria uma estrutura de pedidio*/
struct request_t* startQueue(struct request_t* queue){
  queue = (struct request_t*) malloc(sizeof(struct request_t));
  queue -> op_n = -1;
  queue -> op = -1;
  queue -> key = NULL;
  queue -> data  = NULL;
  queue -> next = NULL;
  return queue;
}

/* Funçao que adiciona uma request ah lista de requests e retorna o numero do request
*/
int add_request_to_queue(MessageT *msg) {
  int result = 0;
  struct request_t *tail;

  pthread_mutex_lock(&queue_lock);
  if (queue_head == NULL){
    queue_head = startQueue(queue_head);

    if (queue_head == NULL){
      pthread_mutex_unlock(&queue_lock);
      return -1;
    }

    result = add_request_to_queue_aux(queue_head, msg);
  } else {
    tail = queue_head;
    while (tail -> next != NULL)
      tail = tail -> next;
    tail -> next = startQueue(tail -> next);

    if (tail -> next == NULL){
      pthread_mutex_unlock(&queue_lock);
      return -1;
    }

    result = add_request_to_queue_aux(tail -> next, msg);
  }

  pthread_cond_signal(&queue_not_empty); /* Avisa um bloqueado nessa condição */
  pthread_mutex_unlock(&queue_lock);

  return result;
}

void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void *watcherCtx) {
  if (type == ZOO_SESSION_EVENT) {
    if (state == ZOO_CONNECTED_STATE) {
      is_connected = 1;
    } else {
      is_connected = 0;
    }
  }
}

char* get_next_server(){
  zoo_string* children_list; //lista de filhos
  children_list = (zoo_string *)malloc(sizeof(zoo_string));

  if (ZOK != zoo_wget_children(zh, root_path, child_watcher, watcher_ctx, children_list)){
    printErrorMessage("Error setting watch");
    return NULL;
  }

  int strCmpResult;
  char *idcpy = strdup(my_id);
  strtok(idcpy, "/");
  char* node = strtok(NULL, "/");
  char *next_server_checker = children_list -> count > 0 ? children_list -> data[0] : node;

  for(int i = 1; i < children_list -> count; i++){
    if(next_server_checker != NULL){
      strCmpResult = strcmp(node, children_list -> data[i]);
      if (strCmpResult < 0){
        int cmpId = strcmp(next_server_checker, children_list -> data[i]);
        int cmpEqual = strcmp(next_server_checker, node);
        if (cmpId > 0)
          next_server_checker = children_list -> data[i];
        else if (cmpEqual <= 0)
          next_server_checker = children_list -> data[i];
      }
    }
 }

 free(idcpy);
 free(children_list);
 return next_server_checker;
}

void replicateChild(){
  while (rtree_verify(next_server, sending -> op_n) != 1){
    if (sending -> op == 1){
      struct entry_t *entry = entry_create(sending -> key, sending -> data);
      rtree_put(next_server, entry);
      free(entry);
      sleep(2);
    }
    else if (sending -> op == 0)
      rtree_del(next_server, sending -> key);
    sleep(2);
  }
  return;
}

void reconnect(char* next_server_checker){

  char* id_next_server;

  if(strcmp(next_server_checker, my_id) != 0){
    id_next_server = malloc(strlen(root_path) * sizeof(char) + 2 + strlen(next_server_checker));
    sprintf(id_next_server, "%s/%s", root_path, next_server_checker);
  }

  char* addr = malloc(zoo_data_len * sizeof(char) + 1);
  if(ZOK == zoo_get(zh, id_next_server, 0, addr, &zoo_data_len, NULL)){
    char *addrCp = strdup(addr);

    if(next_server != NULL){
      if (strcmp(my_id, id_next_server) == 0){
        rtree_disconnect(next_server);
        next_server = NULL;
      }
      else if(strcmp(addrCp, next_server -> ip) != 0){
        rtree_disconnect(next_server);
        next_server = rtree_connect(addrCp);
      }

      if (next_server != NULL && sending -> op_n != -1){
        pthread_mutex_lock(&queue_sending);
        replicateChild();
        pthread_mutex_unlock(&queue_sending);
      }
    } else{
      if(strcmp(my_id, id_next_server) < 0)
        next_server = rtree_connect(addrCp);
      else if (strcmp(my_id, id_next_server) == 0) {
        rtree_disconnect(next_server);
        next_server = NULL;
      }
    }
    free(addrCp);
  } else
    printErrorMessage("Error with the getter\n");

  free(addr);
  free(id_next_server);
}

void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx) {

  if (state == ZOO_CONNECTED_STATE) {
    if (type == ZOO_CHILD_EVENT) {

      char* next_server_checker = get_next_server();

      if (next_server_checker != NULL)
        reconnect(next_server_checker);
    }
  }
}

int tree_init_zoo_server(char* ip, char* port, char*zoo_ip, char* zoo_port){
  char *zoo_addr = malloc(strlen(zoo_ip) * sizeof(char) + 2 + strlen(zoo_port) * sizeof(char));

  char *addr = malloc(strlen(ip) * sizeof(char) + 2 + strlen(port) * sizeof(char));
  sprintf(addr, "%s:%s", ip, port);
  sprintf(zoo_addr, "%s:%s", zoo_ip, zoo_port);

  zh = zookeeper_init(zoo_addr, connection_watcher, 2000, 0, 0, 0);
  if(zh == NULL){
    fprintf(stderr, "Error connecting to ZooKeeper server\n");
    return -1;
  }
  free(zoo_addr);

  if (ZOK != zoo_exists(zh, root_path, 0, NULL)) {
    printf("The node %s does not exist\n", root_path);

    // se o chain nao existir ent vai criar-lo
    if(ZOK == zoo_create(zh, root_path, NULL, -1, &ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0)){
      printMessage("The normal node was created\n");
    }else
      printErrorMessage("Failed to create the normal node \n");
  }

  char node_path[120] = "";
	strcat(node_path, root_path);
	strcat(node_path, "/node");
	int new_path_len = 1024;
	char* new_path = malloc (new_path_len);

	if (ZOK != zoo_create(zh, node_path, addr, strlen(addr)+1, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL | ZOO_SEQUENCE, new_path, new_path_len)) {
	   fprintf(stderr, "Error creating znode from path %s!\n", node_path);
     free(addr);
     free(new_path);
		 exit(EXIT_FAILURE);
	} else {
    my_id = strdup(new_path);
    free(addr);
    free(new_path);
    fprintf(stderr, "Ephemeral Sequencial ZNode created! ZNode path: %s\n", my_id);
    zoo_string* children_list;
    children_list = (zoo_string *)malloc(sizeof(zoo_string));

    if (ZOK != zoo_wget_children(zh, root_path, &child_watcher, watcher_ctx, children_list)) {
      printErrorMessage("Error setting watch");
    }

    char* next_server_checker = NULL;
    for(int i = 0; i < children_list -> count; i++){
      int strCmpResult;
      if(next_server_checker != NULL){
        strCmpResult = strcmp(next_server_checker, children_list -> data[i]);
        if (strCmpResult > 0){
          int cmpId = strcmp(my_id, children_list -> data[i]);
          if(cmpId < 0)
            next_server_checker = children_list -> data[i];
        } else
          next_server_checker = children_list -> data[i];

      } else{
        strCmpResult = strcmp(my_id, children_list -> data[i]);
        if (strCmpResult < 0)
          next_server_checker = children_list -> data[i];
      }
    }

    char* id_next_server = malloc(strlen(root_path) * sizeof(char) + 2 + strlen(next_server_checker));
    sprintf(id_next_server, "%s/%s", root_path, next_server_checker);
    char* address = malloc(zoo_data_len * sizeof(char) + 1);
    if(ZOK == zoo_get(zh, id_next_server, 0, address, &zoo_data_len, NULL)){
      char *addrCp = strdup(address);
      if(next_server != NULL && strcmp(next_server -> ip, addrCp) != 0){
        next_server = rtree_connect(addrCp);
      }
      else{
        if(children_list -> count == 1)
          my_server = rtree_connect(addrCp);
        next_server = NULL;
      }
      free(addrCp);
    } else
      printErrorMessage("Error with the getter\n");

    free(id_next_server);
    free(address);
    free(children_list);
  }
	return 0;
}
