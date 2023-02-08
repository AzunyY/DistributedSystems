/*Grupo que desenvolveu o projeto
*  - Grupo SD - 032
*
*  - 53563 Ana Luís
*  - 56299 Gabriel Gomes Gameiro
*  - 56321 Rodrigo da Silva Antunes
**/

#ifndef _TREE_PRIVATE_H
#define _TREE_PRIVATE_H

#include "tree.h"
#include "entry.h"

struct tree_t {

  struct entry_t *data;   // entrada da arvore
	struct tree_t *left, *right;    // no esquerdo e direito
  char* rootKey;
};

/* Função para remover um elemento da árvore, indicado pela chave key.
* Retorna a raiz da nova árvore corrigida
*/
struct tree_t *delete_recursion(struct tree_t *tree, char *key);

/*
* Funcao recursiva para ir buscar os values - INORDER
*/
void get_data_recursion(int data, struct tree_t *tree, char** keys, void **values, int *index);

/*
 * Função que devolve a entrada maior mais pequena
 */
struct entry_t *get_largest_child(struct tree_t *tree);

void connectZookeeper(char* address);

#endif
