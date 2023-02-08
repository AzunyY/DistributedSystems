/*Grupo que desenvolveu o projeto
*  - Grupo SD - 032
*
*  - 53563 Ana Luís
*  - 56299 Gabriel Gomes Gameiro
*  - 56321 Rodrigo da Silva Antunes
**/

#include "message-private.h"
#include "inet.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sdmessage.pb-c.h>

/*
*Imprime mensagem de erro
*/
void printErrorMessage(char* msg){

    printf("%s\n", WARNING_DELIMITER);
    printf("%s%s%s !!! %s !!!\n", SPACE,SPACE,SPACE, msg);
    printf("%s\n", WARNING_DELIMITER);

}


/*
*Imprime mensagem
*/
void printMessage(char* msg){

    printf("%s\n", WARNING_DELIMITER);
    printf("%s%s%s >>> %s <<<\n", SPACE,SPACE,SPACE, msg);
    printf("%s\n", WARNING_DELIMITER);

}

/*
* Serializa mensagem
*/
int message_to_buf(MessageT *msg, uint8_t **buf) {
    unsigned len;

    len = message_t__get_packed_size(msg);
    *buf = (uint8_t*) malloc(len);

    if (*buf == NULL) {
        fprintf(stdout, "malloc error\n");
        return -1;
    }

    message_t__pack(msg, *buf);
    return len;
}

/*
* Descerializa mensagem
*/
MessageT* buf_to_message(uint8_t **buf, int len) {
  return message_t__unpack(NULL, len, *buf);
}

int write_all(int sock, uint8_t *buf, int len) {
  int bufsize = len;
  while (len > 0) {
    int res = write(sock, buf, len);
    if (res < 0) {
      if (errno == EINTR) continue;
        perror("write failed:");
        return res;
      }
      buf += res;
      len -= res;
  }

  return bufsize;
}

int read_all(int sock, uint8_t *buf, int len) {
  int bufsize = len;
  while (len > 0) {
    int res = read(sock, buf, len);
    if (res < 0) {
      if (errno == EINTR) continue;
        perror("read failed:");
        return res;
    }
      buf += res;
      len -= res;
  }

  return bufsize;
}
