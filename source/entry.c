/*Grupo que desenvolveu o projeto
*  - Grupo SD - 032
*
*  - 53563 Ana Luís
*  - 56299 Gabriel Gomes Gameiro
*  - 56321 Rodrigo da Silva Antunes
**/

#include "entry.h"
#include "string.h"
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

/* Função que cria uma entry, reservando a memória necessária para a
 * estrutura e inicializando os campos key e value, respetivamente, com a
 * string e o bloco de dados passados como parâmetros, sem reservar
 * memória para estes campos.

 */
struct entry_t *entry_create(char *key, struct data_t *data){

  struct entry_t *entry = (struct entry_t*) malloc(sizeof(struct entry_t));

  if (entry == NULL)
    return NULL;

  entry -> key = key;
  entry -> value = data;

  return entry;

}

/* Função que elimina uma entry, libertando a memória por ela ocupada
 */
void entry_destroy(struct entry_t *entry){

  if (entry != NULL) {
    free(entry -> key);
    data_destroy(entry -> value);
    free(entry);
  }

}

/* Função que duplica uma entry, reservando a memória necessária para a
 * nova estrutura.
 */
struct entry_t *entry_dup(struct entry_t *entry){

  if (entry == NULL)
    return NULL;

  if (entry -> key == NULL && entry -> value == NULL)
    return entry_create(entry -> key, entry -> value);

  char *duplicatedKey = strdup(entry -> key);
  struct data_t *duplicatedData = data_dup(entry -> value);
  struct entry_t *entryDuplicate = entry_create(duplicatedKey, duplicatedData);

  if (entryDuplicate == NULL)
    return NULL;

  return entryDuplicate;
}

/* Função que substitui o conteúdo de uma entrada entry_t.
*  Deve assegurar que destroi o conteúdo antigo da mesma.
*/
void entry_replace(struct entry_t *entry, char *new_key, struct data_t *new_value){


  //se entry for null nao substituir nada
  if (entry == NULL)
    return;

  //cria entry
  if (new_key == NULL && new_value == NULL) {
    entry_create(new_key, new_value);
    return;
  }

  //se so um dos valores for null, nao substitui nada
  if (new_key == NULL || new_value == NULL) {
    perror("It is not possible to replace");
    return;
  }

  free(entry -> key);
  data_destroy(entry -> value);

  entry -> key = new_key;
  entry -> value = new_value;
}

/* Função que compara duas entradas e retorna a ordem das mesmas.
*  Ordem das entradas é definida pela ordem das suas chaves.
*  A função devolve 0 se forem iguais, -1 se entry1<entry2, e 1 caso contrário.
*/
int entry_compare(struct entry_t *entry1, struct entry_t *entry2){

  if (entry1 == NULL || entry2 == NULL) {
    perror("Wasn't possible to compare the entries");
    exit(1);
  }

  int cmp = strcmp(entry1 -> key, entry2 -> key);

  if (cmp == 0)
    return 0;
  else if (cmp > 0)
    return 1;
  else
    return -1;
}
