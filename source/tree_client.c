/*Grupo que desenvolveu o projeto
*  - Grupo SD - 032
*
*  - 53563 Ana Luís
*  - 56299 Gabriel Gomes Gameiro
*  - 56321 Rodrigo da Silva Antunes
**/

#include "client_stub-private.h"
#include "zookeeper/zookeeper.h"
#define ZDATALEN 1024 * 1024

int zoo_data_len = ZDATALEN;
char* zoo_root = "/chain";
char* headserver;
char* tailserver;
char *watcher_ctx = "ZooKeeper watch chain";
static zhandle_t *zh;
struct rtree_t* head;
struct rtree_t* tail;
int messageSend;
char* keySend;
int codeSend;
struct entry_t* entrySend;

typedef struct String_vector zoo_string;

/*
* Imprime uma mensagem de ajuda com os comandos validos do programa
*/
void print_help_message()
{
  printf("| Commands: \n");
  printf("%s Insert new entry - put <ckey> <value>\n", SPACE);
  printf("%s Delete key - del <key>\n", SPACE);
  printf("%s Get value - get <key>\n", SPACE);
  printf("%s Heigh - height\n", SPACE);
  printf("%s Size - size\n", SPACE);
  printf("%s Get all values - getvalues\n", SPACE);
  printf("%s Get all keys - getkeys)\n", SPACE);
  printf("%s Verify - verify <op_n>)\n", SPACE);
  printf("%s Quit - quit\n", SPACE);
}

/*
* Trata do ctrlC
*/
void signalhandler(){
  rtree_disconnect(head);
  rtree_disconnect(tail);
  if(keySend != NULL)
    free(keySend);
  zookeeper_close(zh);
  exit(-1);
}

void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx) {

  if (state == ZOO_CONNECTED_STATE) {
    if (type == ZOO_CHILD_EVENT) {

      zoo_string* children_list = (zoo_string *)malloc(sizeof(zoo_string));
      children_list -> count = 0;
      children_list -> data = NULL;
      if (ZOK != zoo_wget_children(zh, zoo_root, child_watcher, watcher_ctx, children_list))
        fprintf(stderr, "Error setting watch at %s!\n", "/chain");
      else {
        char* head_server = NULL;
        char* tail_server = NULL;
        for (int i = 0; i < children_list->count; i++) {

          if(tail_server != NULL && head_server != NULL){
            int strcmpReslt = strcmp(head_server, children_list -> data[i]);
            if(strcmpReslt > 0)
              head_server = children_list -> data[i];

            strcmpReslt = strcmp(tail_server, children_list -> data[i]);
            if(strcmpReslt < 0)
              tail_server = children_list -> data[i];
          }

          if(tail_server == NULL)
            tail_server = children_list -> data[i];

          if(head_server == NULL)
            head_server = children_list -> data[i];
        }

        free(children_list);

        if(tail_server != NULL){
          char *addr = malloc(zoo_data_len* sizeof(char));
          char *id = malloc(strlen("/chain/") + 1 + strlen(tail_server));
          sprintf(id, "/chain/%s", tail_server);
          if(ZOK == zoo_get(zh, id, 0, addr, &zoo_data_len, NULL)){
            int strCmpResult = 0;

            if(tailserver != NULL)
              strCmpResult = strcmp(tailserver, tail_server);
            else{
              tailserver = tail_server;
              tail = rtree_connect(addr);
            }

            if (strCmpResult != 0){
              rtree_disconnect(tail);
              tailserver = tail_server;
              tail = rtree_connect(addr);
            }
          } else
            fprintf(stderr, "Error with the data getter tail \n");
          free(addr);
          free(id);
        }

        if(head_server != NULL){
          char *addr = malloc(zoo_data_len * sizeof(char));
          char *id = malloc(strlen("/chain/") + 1 + strlen(head_server));
          sprintf(id, "/chain/%s", head_server);
          if( ZOK == zoo_get(zh, id, 0, addr, &zoo_data_len,NULL)){
            int strCmpResult = 0;

            if(headserver != NULL)
              strCmpResult = strcmp(headserver, head_server);
            else{
              headserver = head_server;
              head = rtree_connect(addr);
            }

            if (strCmpResult != 0){
              rtree_disconnect(head);
              headserver = head_server;
              head = rtree_connect(addr);
            }

          } else
            fprintf(stderr, "Error with the data getter head \n");
          free(addr);
          free(id);
        }

        if (messageSend == 0){
          while (rtree_verify(head, codeSend) != 1) {
            printErrorMessage("It was not possible to send the message\n");
            codeSend = rtree_put(head, entrySend);
            sleep(2);
          }
        } else if(messageSend == 1){
          while (rtree_verify(head, codeSend) != 1) {
            printErrorMessage("It was not possible to send the message\n");
            codeSend = rtree_del(head, keySend);
            sleep(2);
          }
        }
      }
    }
  }
}

void connectZookeeper(char* address){

  zh = zookeeper_init(address, NULL, 2000, 0, NULL, 0);
  if (zh == NULL) {
    fprintf(stderr, "Error connecting to ZooKeeper server!\n");
    exit(EXIT_FAILURE);
  }

  zoo_string* children_list = (zoo_string *)malloc(sizeof(zoo_string));
  children_list -> count = 0;
  children_list -> data = NULL;
  if (ZOK != zoo_wget_children(zh, "/chain", child_watcher, watcher_ctx, children_list))
    printf("Error setting watch at %s!\n", "/chain");

  char* head_server = NULL;
  char* tail_server = NULL;

  for (int i = 0; i < children_list -> count; i++) {

    if(tail_server != NULL && head_server != NULL){
      int strcmpReslt = strcmp(head_server, children_list -> data[i]);
      if(strcmpReslt > 0)
        head_server = children_list -> data[i];

      strcmpReslt = strcmp(tail_server, children_list -> data[i]);
      if(strcmpReslt < 0)
        tail_server = children_list -> data[i];
    }

    if(tail_server == NULL)
      tail_server = children_list -> data[i];

    if(head_server == NULL)
      head_server = children_list -> data[i];
  }

  free(children_list);

  if(tail_server != NULL){
    char *addr = malloc(zoo_data_len * sizeof(char));
    char *id = malloc(strlen("/chain/") + 1 + strlen(tail_server));
    sprintf(id, "/chain/%s", tail_server);
    if( ZOK != zoo_get(zh, id, 0, addr, &zoo_data_len,NULL))
      fprintf(stderr, "Error with the data getter tail");
    else
      tail = rtree_connect(addr);
    free(id);
    free(addr);
    tailserver = tail_server;
  }

  if(head_server != NULL){
    char *addr = malloc(zoo_data_len * sizeof(char));
    char *id = malloc(strlen("/chain/") + 1 + strlen(head_server));
    sprintf(id, "/chain/%s", head_server);
    if( ZOK != zoo_get(zh, id, 0, addr, &zoo_data_len,NULL))
      fprintf(stderr, "Error with the data getter head");
    else
      head = rtree_connect(addr);
    free(id);
    free(addr);
    headserver = head_server;
  }
  return;
}

/*
* Main do cliente
*/
int main(int argc, char **argv) {
  signal(SIGPIPE, SIG_IGN);
  signal(SIGINT, signalhandler);
  messageSend = -1;
  entrySend = NULL;
  keySend = NULL;

  if (argc != 2){
    printf("Use: ./client_example <ip_zookeeper:port_zookeeperr> \n");
    printf("Use example: ./client_example 127.0.0.1:2181\n");
    return -1;
  }

  print_help_message();

  //RECEBE OS ARGUMENTOS
  char address[100];
  sprintf(address, "%s", argv[1]);

  connectZookeeper(address);

  char text[100];
  while (1) {

    fgets(text, 100, stdin);
    text[strlen(text) - 1] = '\0';

    if (strcmp(text, "\0") == 0)
      continue;

    if (text == NULL)
      return -1;

    char *comand = strtok(text, " ");
    if (strcmp(comand, "quit") == 0){
      if(keySend != NULL)
        free(keySend);
      messageSend = -1;
      rtree_disconnect(tail);
      rtree_disconnect(head);
      zookeeper_close(zh);
      break;
    }

    //PUT
    else if (strcmp(comand, "put") == 0) {
      /* strtok a primeira chamada de strtok e quando se faz strtok(text, " ")
      * as proximas chamadas usam NUll para informar a funcao que deve
      * continuar a tokanizar a string que foi passada inicialmente - txt
      * Ou seja:
      * 1 call --> 1 token, 2 call --> 2 token, e assim adiante
      */
      messageSend = 0;
      comand = strtok(NULL, " ");
      if (comand == NULL) {
        printErrorMessage("Wrong number of args");
        continue;
      }
      char *key = strdup(comand);
      comand = strtok(NULL, "\0");
      if (comand == NULL) {
        free(key);
        printErrorMessage("Wrong number of args");
        continue;
      }
      char *data = strdup(comand);
      struct data_t *value = data_create2(strlen(data)+1, data);
      struct entry_t *entry = entry_create(key, value);
      int code = rtree_put(head, entry);
      entrySend = entry_dup(entry);
      if(entrySend != NULL)
        entry_destroy(entrySend);
      if(keySend != NULL)
        free(keySend);
      sleep(2);

      while(rtree_verify(head, code) != 1){
        code = rtree_put(head, entry);
        sleep(2);
      }

      keySend = NULL;
      codeSend = code;

      printf("%s\n", WARNING_DELIMITER);
      printf(" >>>>>> Code of put request: %d\n", code);
      printf("%s\n", WARNING_DELIMITER);
      entry_destroy(entry);
    }

    //GET
    else if (strcmp(comand, "get") == 0) {
      messageSend = -1;

      char *comand = strtok(NULL, "\0");

      if (comand == NULL) {
        printErrorMessage("Wrong number of args");
        continue;
      }

      struct data_t *data = rtree_get(tail, comand);
      if (data == NULL)
        printErrorMessage("Key does not exist");
      else{
        printf("%s\n", WARNING_DELIMITER);
        printf(" >>>>>> Data: %s\n", (char*) data -> data);
        printf("%s\n", WARNING_DELIMITER);
      }

      data_destroy(data);
    }

    //DELETE
    else if (strcmp(comand, "del") == 0) {

      char *comand = strtok(NULL, "\0");
      if (comand == NULL) {
        printErrorMessage("Wrong number of args");
        continue;
      }

      messageSend = 1;
      int code = rtree_del(head, comand);
      sleep(2);

      if(keySend != NULL)
        free(keySend);

      while(rtree_verify(head, code) != 1){
        code = rtree_del(head, comand);
        sleep(2);
      }

      keySend = strdup(comand);
      entrySend = NULL;
      codeSend = code;

      printf("%s\n", WARNING_DELIMITER);
      printf(" >>>>>> Code of delete request: %d\n", code);
      printf("%s\n", WARNING_DELIMITER);
    }

    //SIZE
    else if (strcmp(comand, "size") == 0){
      messageSend = -1;

      printf("%s\n", WARNING_DELIMITER);
      printf(" >>>>>> Size: %d\n", rtree_size(tail));
      printf("%s\n", WARNING_DELIMITER);
    }

    //HEIGHT
    else if (strcmp(comand, "height") == 0){
      messageSend = -1;

      printf("%s\n", WARNING_DELIMITER);
      printf(" >>>>>> Height: %d\n", rtree_height(tail));
      printf("%s\n", WARNING_DELIMITER);
    }

    //GETKEYS
    else if (strcmp(comand, "getkeys") == 0) {
      messageSend = -1;

      char **keys = rtree_get_keys(tail);

      if (keys == NULL)
        printErrorMessage("Keys do not exist");

      else{
        int i = 0;
        printf("%s\n", WARNING_DELIMITER);
        while(keys[i] != NULL){
          printf(" >>>>>>> Key: %s\n", keys[i]);
          i++;
        }
        printf("%s\n", WARNING_DELIMITER);
        i = 0;

        while(keys[i] != NULL){
          free(keys[i]);
          i++;
        }
      }
      free(keys);
    }

    //GETVALUES
    else if (strcmp(comand, "getvalues") == 0) {
      messageSend = -1;

      void **values = rtree_get_values(tail);
      if (values == NULL)
        printErrorMessage("Values do not exist");

      else{
        int i = 0;
        printf("%s\n", WARNING_DELIMITER);
        while (values[i] != NULL) {
          printf(" >>>>>> Value: %s\n", (char*) values[i]);
          i++;
        }
        printf("%s\n", WARNING_DELIMITER);

        i = 0;
        while(values[i] != NULL){
          free(values[i]);
          i++;
        }
      }
      free(values);
    }
    else if (strcmp(comand, "verify") == 0) {
      messageSend = -1;

      char *comand = strtok(NULL, "\0");

      if (comand == NULL) {
        printErrorMessage("Wrong number of arguments");
        continue;
      }

      int r = rtree_verify(tail, atoi(comand));
      if (r == 1)
        printMessage("Operation executed");
      else if(r == 0)
        printErrorMessage("Operation was not executed");
      else
        printErrorMessage("Operation was not assigned");
      }
    }
  return 0;
}
