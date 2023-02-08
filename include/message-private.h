/*Grupo que desenvolveu o projeto
*  - Grupo SD - 032
*
*  - 53563 Ana Luís
*  - 56299 Gabriel Gomes Gameiro
*  - 56321 Rodrigo da Silva Antunes
**/

#ifndef _MESSAGE_H
#define _MESSAGE_H

#include <stdint.h>
#include "inet.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sdmessage.pb-c.h>

#define WARNING_DELIMITER "-----------------------------------------------------"
#define SPACE "   "


/*
* Serializa mensagem
*/
int message_to_buf(MessageT* msg, uint8_t** buf);

/*
* Descerializa mensagem
*/
MessageT* buf_to_message(uint8_t** buf, int len);

int write_all(int sock, uint8_t *buf, int len);

int read_all(int sock, uint8_t *buf, int len);

/*
*Imprime mensagem de erro
*/
void printErrorMessage(char* msg);

/*
*Imprime mensagem
*/
void printMessage(char* msg);

#endif
