/*Grupo que desenvolveu o projeto
*  - Grupo SD - 032
*
*  - 53563 Ana Luís
*  - 56299 Gabriel Gomes Gameiro
*  - 56321 Rodrigo da Silva Antunes
**/

#include "network_server-private.h"

int sockfd;
struct pollfd* desc_set;
extern int sair; //Variavel para sair do ctrlC
extern char* server_host;
extern char* port_server;
extern char* zoo_host;
extern char* port_zoo;
int NFDESC = 4;

/* Função para preparar uma socket de receção de pedidos de ligação
 * num determinado porto.
 * Retornar descritor do socket (OK) ou -1 (erro).
 */
int network_server_init(short port) {
  struct sockaddr_in server;

  // Cria socket TCP
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Error creating socket\n");
    return -1;
  }

  int reuse = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)
    perror("setsockopt(SO_REUSEADDR) failed\n");

  // Preenche estrutura server com endereço(s) para associar (bind) à socket
  server.sin_family = AF_INET;
  server.sin_port = htons(port);               // Porta TCP
  server.sin_addr.s_addr = htonl(INADDR_ANY);  // Todos os endereços na máquina

  // Faz bind, ver observacao
  if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
    perror("Error binding\n");
    close(sockfd);
    return -1;
  }

  // Esta chamada diz ao SO que esta é uma socket para receber pedidos
  if (listen(sockfd, 0) < 0) {
    perror("Error listening\n");
    close(sockfd);
    return -1;
  }

  return sockfd;
}


/*Funcao que inicializa valores de desc_set*/
void setDesc(int i){
  for (i = i; i < NFDESC; i++){
    desc_set[i].fd = -1;  // poll ignora estruturas com fd < 0
    desc_set[i].events = POLLIN;
    desc_set[i].revents = 0;
  }
}

/* Esta função deve:
 * - Aceitar uma conexão de um cliente;
 * - Receber uma mensagem usando a função network_receive;
 * - Entregar a mensagem de-serializada ao skeleton para ser processada;
 * - Esperar a resposta do skeleton;
 * - Enviar a resposta ao cliente usando a função network_send.
 */
int network_main_loop(int listening_socket) {

  struct sockaddr_in client;
  socklen_t size_client = 0;

  int nfds, kfds, i;
  desc_set = malloc(sizeof(struct pollfd)* NFDESC);  // Estrutura para file descriptors das sockets das ligacoes
  setDesc(0);

  // adiciona listening_socket a desc_set
  desc_set[0].fd = listening_socket;
  desc_set[0].events = POLLIN;  // Vamos esperar ligacoes nesta socket
  nfds = 1;
  int used = 0;

  if (tree_init_zoo_server(server_host, port_server, zoo_host, port_zoo) == -1) {
    printErrorMessage("Error initializing ZooKeeper\n");
    return -1;
  }

  printMessage("Server waiting...");

  //ciclo fila de espera
  while (!sair && (kfds = poll(desc_set, nfds, TIMEOUT)) >= 0) { // kfds == 0 significa timeout sem eventos

    if (kfds > 0) {  // kfds e o numero de descritores com evento ou erro
      if (desc_set[0].revents & POLLIN){ // Pedido na listening socket
        if(used != 0){
          desc_set[used].fd = accept(desc_set[0].fd, (struct sockaddr *)&client, &size_client);
          used = 0;
        } else{
          if ((desc_set[nfds].fd = accept(desc_set[0].fd, (struct sockaddr *)&client, &size_client)) > 0) // Ligacao feita
            nfds++;
        }
      }

      if(nfds >= NFDESC){
        for(int j = 0; j < nfds; j++){
          if(desc_set[j].fd == -1)
              used = j;
        }

        if(used == 0){
          used = 0;
          struct pollfd* oldDesc = realloc(desc_set, 2*NFDESC*sizeof(struct pollfd));
          desc_set = oldDesc;
          int oldValue = NFDESC;
          NFDESC = 2*NFDESC;
          setDesc(oldValue);
        }
    }

      for (i = 1; i < nfds; i++) {  // Todas as ligacoes
        if (desc_set[i].revents & POLLIN) {  // Dados para ler
          MessageT *msg = network_receive(desc_set[i].fd);

          if (msg == NULL) {
            printMessage("client ended connection");
            close(desc_set[i].fd);
            desc_set[i].fd = -1;
            message_t__free_unpacked(msg, NULL);
          } else {
            printErrorMessage("Request received");
            invoke(msg);

            if (network_send(desc_set[i].fd, msg) == -1) {
              close(desc_set[i].fd);
              desc_set[i].fd = -1;
              continue;  // continuar iteração do for
            }
            printMessage("Response sent");
          }

          message_t__free_unpacked(msg, NULL);
          printMessage("Waiting for a request");
        }

        if (desc_set[i].revents & POLLHUP || desc_set[i].revents & POLLERR) {
          close(desc_set[i].fd);
          desc_set[i].fd = -1;
        }
      }
    }
  }

  return 0;
}


/* Esta função deve:
 * - Ler os bytes da rede, a partir do client_socket indicado;
 * - De-serializar estes bytes e construir a mensagem com o pedido,
 *   reservando a memória necessária para a estrutura message_t.
 */
MessageT* network_receive(int client_socket) {

  int len;
  uint8_t *lenS = (uint8_t *) &len;
  int rcvdBytes;

  if ((rcvdBytes = read_all(client_socket, lenS, sizeof(int))) < 0) {
    perror("Error receiving data from client\n");
    return NULL;
  }

  len = ntohl(len);
  if (len == -1)
    return NULL;

  uint8_t *buf = (uint8_t *) malloc(len);

  if ((rcvdBytes = read_all(client_socket, buf, len)) < 0) {
    perror("Error receiving data from client\n");
    return NULL;
  }

  MessageT *msg = buf_to_message(&buf, len);

  free(buf);
  return msg;
}

/* Esta função deve:
 * - Serializar a mensagem de resposta contida em msg;
 * - Libertar a memória ocupada por esta mensagem;
 * - Enviar a mensagem serializada, através do client_socket.
 */
int network_send(int client_socket, MessageT* msg) {
  uint8_t *buf;
  int len = message_to_buf(msg, &buf);

  if (len < 0) {
    perror("Error serializing to client \n");
    return -1;
  }

  int lenToSend = htonl(len);
  uint8_t *lenS = (uint8_t *)&lenToSend;

  // Envia tamanho da msg
  int sentBytes;
  if ((sentBytes = write_all(client_socket, lenS, sizeof(int))) != sizeof(int)) {
    perror("Error sending response to client\n");
    return -1;
  }
  // Envia msg
  if ((sentBytes = write_all(client_socket, buf, len)) != len) {
    perror("Error sending response to client\n");
    return -1;
  }

  free(buf);
  return 0;
}

/* A função network_server_close() liberta os recursos alocados por
 * network_server_init().
 */
int network_server_close() {

  if(desc_set != NULL)
    free(desc_set);

  if(sockfd != -1)
    close(sockfd);

  tree_skel_destroy();
  return 0;
}
