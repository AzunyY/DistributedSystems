/*Grupo que desenvolveu o projeto
*  - Grupo SD - 032
*
*  - 53563 Ana Luís
*  - 56299 Gabriel Gomes Gameiro
*  - 56321 Rodrigo da Silva Antunes
**/

#include "client_stub-private.h"

/* Função para estabelecer uma associação entre o cliente e o servidor,
 * em que address_port é uma string no formato <hostname>:<port>.
 * Retorna NULL em caso de erro.
 */
struct rtree_t *rtree_connect(const char *address_port) {

  struct rtree_t *rtree = (struct rtree_t*) malloc(sizeof(struct rtree_t));
  if (rtree == NULL)
    return NULL;

  char *copy = strdup(address_port);
  char *copy2 = copy;
  char *host = strtok(copy, ":");
  copy = strtok(NULL, ":");
  char *port = strtok(copy, ":");

  // Preenche estrutura server com endereço do servidor para estabelecer
  // conexão
  rtree -> server.sin_family = AF_INET;          // família de endereços internet
  rtree -> server.sin_port = htons(atoi(port));
  rtree -> ip = strdup(address_port);

   // Porta TCP
  //SOL_SOCKET e informar que e uma ligacao tcp
  if (inet_pton(AF_INET, host, &rtree -> server.sin_addr) < 1) {  // Endereço IP
    printErrorMessage("Error IP converting");
    free(copy2);
    return NULL;
  }

  if (network_connect(rtree) < 0){
    free(copy2);
    free(rtree);
    return NULL;
  }

  free(copy2);
  printMessage("Connected");
  return rtree;
}

/* Termina a associação entre o cliente e o servidor, fechando a
 * ligação com o servidor e libertando toda a memória local.
 * Retorna 0 se tudo correr bem e -1 em caso de erro.
 */
int rtree_disconnect(struct rtree_t *rtree) {

    if(rtree == NULL)
      return -1;

    network_send_receive(rtree, NULL);

    if (close(rtree -> socket) == -1) {
      free(rtree -> ip);
      free(rtree);
      return -1;
    }

    free(rtree -> ip);
    free(rtree);
    return 0;
}

/* Função para adicionar um elemento na árvore.
 * Se a key já existe, vai substituir essa entrada pelos novos dados.
 * Devolve 0 (ok, em adição/substituição) ou -1 (problemas).
 */
int rtree_put(struct rtree_t *rtree, struct entry_t *entry) {

  if (rtree == NULL || entry == NULL)
    return -1;

  MessageT msg_proto;
  message_t__init(&msg_proto);

  msg_proto.opcode = MESSAGE_T__OPCODE__OP_PUT;
  msg_proto.c_type = MESSAGE_T__C_TYPE__CT_ENTRY;
  //entry
  msg_proto.key = entry -> key;
  msg_proto.datasize = entry -> value -> datasize;
  msg_proto.data = (char* ) entry -> value -> data;
  msg_proto.result = -1;
  msg_proto.n_keys = 0;
  msg_proto.n_values = 0;
  msg_proto.keys = NULL;
  msg_proto.values = NULL;

  MessageT* receive = network_send_receive(rtree, &msg_proto);

  if (receive == NULL || receive -> opcode == MESSAGE_T__OPCODE__OP_ERROR){
    message_t__free_unpacked(receive, NULL);
    return -1;
  }

  int re = receive -> result;

  message_t__free_unpacked(receive, NULL);
  return re;
}

/* Função para obter um elemento da árvore.
 * Em caso de erro, devolve NULL.
 */
struct data_t *rtree_get(struct rtree_t *rtree, char *key) {

  if (rtree == NULL || key == NULL)
    return NULL;

  MessageT msg_proto;
  message_t__init(&msg_proto);

  msg_proto.opcode = MESSAGE_T__OPCODE__OP_GET;
  msg_proto.c_type = MESSAGE_T__C_TYPE__CT_KEY;
  msg_proto.key = key;
  msg_proto.result = -1;
  msg_proto.datasize = 0;
  msg_proto.data = NULL;
  msg_proto.n_keys = 0;
  msg_proto.n_values = 0;
  msg_proto.keys = NULL;
  msg_proto.values = NULL;

  MessageT* receive = network_send_receive(rtree, &msg_proto);

  if (receive -> opcode == MESSAGE_T__OPCODE__OP_ERROR){
    message_t__free_unpacked(receive, NULL);
    return NULL;
  }

  struct data_t* returnData;
  if(receive -> datasize > 0)
    returnData = data_create2(receive -> datasize, strdup(receive -> data));
  else{
    message_t__free_unpacked(receive, NULL);
    return NULL;
  }

  message_t__free_unpacked(receive, NULL);
  return returnData;
}

/* Função para remover um elemento da árvore. Vai libertar
 * toda a memoria alocada na respetiva operação rtree_put().
 * Devolve: 0 (ok), -1 (key not found ou problemas).
 */
int rtree_del(struct rtree_t *rtree, char *key) {

  if (rtree == NULL || key == NULL)
    return -1;


  MessageT msg_proto;
  message_t__init(&msg_proto);

  msg_proto.opcode = MESSAGE_T__OPCODE__OP_DEL;
  msg_proto.c_type = MESSAGE_T__C_TYPE__CT_KEY;
  msg_proto.key = key;
  msg_proto.result = -1;
  msg_proto.datasize = 0;
  msg_proto.data = NULL;
  msg_proto.n_keys = 0;
  msg_proto.n_values = 0;
  msg_proto.keys = NULL;
  msg_proto.values = NULL;

  MessageT* receive = network_send_receive(rtree, &msg_proto);

  if (receive == NULL || receive -> opcode == MESSAGE_T__OPCODE__OP_ERROR){
    message_t__free_unpacked(receive, NULL);
    return -1;
  }

  int re = receive -> result;
  message_t__free_unpacked(receive, NULL);
  return re;
}

/* Devolve o número de elementos contidos na árvore.
 */
int rtree_size(struct rtree_t *rtree) {

  if (rtree == NULL)
    return -1;

  MessageT msg_proto;
  message_t__init(&msg_proto);

  msg_proto.opcode = MESSAGE_T__OPCODE__OP_SIZE;
  msg_proto.c_type = MESSAGE_T__C_TYPE__CT_NONE;
  msg_proto.result = -1;
  msg_proto.datasize = 0;
  msg_proto.data = NULL;
  msg_proto.n_keys = 0;
  msg_proto.n_values = 0;
  msg_proto.keys = NULL;
  msg_proto.values = NULL;
  msg_proto.key = NULL;

  MessageT* receive = network_send_receive(rtree, &msg_proto);

  if (receive == NULL || receive -> opcode == MESSAGE_T__OPCODE__OP_ERROR){
    message_t__free_unpacked(receive, NULL);
    return -1;
  }

  int re = receive -> result;
  message_t__free_unpacked(receive, NULL);
  return re;
}

/* Função que devolve a altura da árvore.
 */
int rtree_height(struct rtree_t *rtree) {

  if (rtree == NULL)
    return -1;


  MessageT msg_proto;
  message_t__init(&msg_proto);

  msg_proto.opcode = MESSAGE_T__OPCODE__OP_HEIGHT;
  msg_proto.c_type = MESSAGE_T__C_TYPE__CT_NONE;
  msg_proto.result = -1;
  msg_proto.datasize = 0;
  msg_proto.data = NULL;
  msg_proto.n_keys = 0;
  msg_proto.n_values = 0;
  msg_proto.keys = NULL;
  msg_proto.values = NULL;
  msg_proto.key = NULL;

  MessageT* receive = network_send_receive(rtree, &msg_proto);

  if (receive == NULL || receive -> opcode == MESSAGE_T__OPCODE__OP_ERROR){
    message_t__free_unpacked(receive, NULL);
    return -1;
  }

  int re = receive -> result;
  message_t__free_unpacked(receive, NULL);
  return re;
}

/* Devolve um array de char* com a cópia de todas as keys da árvore,
 * colocando um último elemento a NULL.
 */
char **rtree_get_keys(struct rtree_t *rtree) {

  if (rtree == NULL)
    return NULL;

  MessageT msg_proto;
  message_t__init(&msg_proto);

  msg_proto.opcode = MESSAGE_T__OPCODE__OP_GETKEYS;
  msg_proto.c_type = MESSAGE_T__C_TYPE__CT_NONE;
  msg_proto.result = -1;
  msg_proto.datasize = 0;
  msg_proto.data = NULL;
  msg_proto.n_keys = 0;
  msg_proto.n_values = 0;
  msg_proto.keys = NULL;
  msg_proto.values = NULL;
  msg_proto.key = NULL;

  MessageT* receive = network_send_receive(rtree, &msg_proto);

  if (receive -> opcode == MESSAGE_T__OPCODE__OP_ERROR || receive -> keys == NULL){
    message_t__free_unpacked(receive, NULL);
    return NULL;
  }

  char** ret = malloc((receive -> n_keys + 1) * sizeof(char *));

  for(int i = 0; i < receive -> n_keys; i++)
    ret[i] = strdup(receive -> keys[i]);

  ret[receive -> n_keys] = NULL;
  message_t__free_unpacked(receive, NULL);
  return ret;
}


/* Devolve um array de void* com a cópia de todas os values da árvore,
 * colocando um último elemento a NULL.
 */
void **rtree_get_values(struct rtree_t *rtree){

  if (rtree == NULL)
    return NULL;

  MessageT msg_proto;
  message_t__init(&msg_proto);

  msg_proto.opcode = MESSAGE_T__OPCODE__OP_GETVALUES;
  msg_proto.c_type = MESSAGE_T__C_TYPE__CT_NONE;
  msg_proto.result = -1;
  msg_proto.datasize = 0;
  msg_proto.data = NULL;
  msg_proto.n_keys = 0;
  msg_proto.n_values = 0;
  msg_proto.keys = NULL;
  msg_proto.values = NULL;
  msg_proto.key = NULL;


  MessageT* receive = network_send_receive(rtree, &msg_proto);

  if (receive -> opcode == MESSAGE_T__OPCODE__OP_ERROR || receive -> values == NULL){
    message_t__free_unpacked(receive, NULL);
    return NULL;
  }

  void** ret = malloc((receive -> n_values + 1) * sizeof(char *));

  for(int i = 0; i < receive -> n_values; i++)
    ret[i] = strdup(receive -> values[i]);

  ret[receive -> n_values] = NULL;
  message_t__free_unpacked(receive, NULL);
  return ret;
}

/* Verifica se a operação identificada por op_n foi executada.
*/
int rtree_verify(struct rtree_t *rtree, int op_n) {
  if (rtree == NULL || op_n < 0)
    return -1;

  MessageT msg_proto;
  message_t__init(&msg_proto);

  msg_proto.opcode = MESSAGE_T__OPCODE__OP_VERIFY;
  msg_proto.c_type = MESSAGE_T__C_TYPE__CT_RESULT;
  msg_proto.result = op_n;
  msg_proto.datasize = 0;
  msg_proto.data = NULL;
  msg_proto.n_keys = 0;
  msg_proto.n_values = 0;
  msg_proto.keys = NULL;
  msg_proto.values = NULL;
  msg_proto.key = NULL;

  MessageT *receive = network_send_receive(rtree, &msg_proto);

  if (receive == NULL){
    message_t__free_unpacked(receive, NULL);
    return -1;
  }

  if (receive -> opcode == MESSAGE_T__OPCODE__OP_ERROR){
    message_t__free_unpacked(receive, NULL);
    return -1;
  }

  int result = receive -> result;
  message_t__free_unpacked(receive, NULL);
  return result;
}
