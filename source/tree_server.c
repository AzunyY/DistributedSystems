/*Grupo que desenvolveu o projeto
*  - Grupo SD - 032
*
*  - 53563 Ana Luís
*  - 56299 Gabriel Gomes Gameiro
*  - 56321 Rodrigo da Silva Antunes
**/

#include "network_server-private.h"

int sair = 0; //Variavel para sair do ctrlC
char*  server_host;
char* port_server;
char* zoo_host;
char* port_zoo;

/*
* Trata do ctrlC
*/
void ctrlC(){
  sair = 1;
}

int main(int argc, char **argv) {
  signal(SIGPIPE, SIG_IGN);
  signal(SIGINT, ctrlC);

  //PERGUNTAR ISTO ou so recebe a porta do servidor e nao o ip delw
  if (argc != 5) {
    printf("Use: ./tree_server <ip> <port> <ipZookeeper><portZookeeper> \n");
    printf("Use example: ./tree_server 127.0.0.1 12345 127.0.0.1 2181\n");
    return -1;
  }

  int port = atoi(argv[2]);

  server_host = argv[1];
  port_server = argv[2];
  zoo_host = argv[3];
  port_zoo = argv[4];

  int sockfd = network_server_init(port);
  if (tree_skel_init() == -1) {
    printErrorMessage("Error initializing tree of server");
    return -1;
  }

  network_main_loop(sockfd);
  network_server_close();

  return 0;
}
