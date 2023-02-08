/*Grupo que desenvolveu o projeto
*  - Grupo SD - 032
*
*  - 53563 Ana Luís
*  - 56299 Gabriel Gomes Gameiro
*  - 56321 Rodrigo da Silva Antunes
**/

#include <stdlib.h>
#include <string.h>
#include "tree-private.h"
#include <stdio.h>


/* Função para criar uma nova árvore tree vazia.
 * Em caso de erro retorna NULL.
 */
struct tree_t *tree_create(){

  struct tree_t *tree = (struct tree_t *)malloc(sizeof(struct tree_t));

  if (tree == NULL)
    return NULL;

  tree -> data = NULL;
  tree -> right = NULL;
  tree -> left = NULL;

  return tree;
}

/* Função para libertar toda a memória ocupada por uma árvore.
 */
void tree_destroy(struct tree_t *tree){

  if (tree == NULL)
    return;

  tree_destroy(tree -> left);
  tree_destroy(tree -> right);
  entry_destroy(tree -> data);

  free(tree);
}

/* Função para adicionar um par chave-valor à árvore.
 * Os dados de entrada desta função deverão ser copiados, ou seja, a
 * função vai *COPIAR* a key (string) e os dados para um novo espaço de
 * memória que tem de ser reservado. Se a key já existir na árvore,
 * a função tem de substituir a entrada existente pela nova, fazendo
 * a necessária gestão da memória para armazenar os novos dados.
 * Retorna 0 (ok) ou -1 em caso de erro.
 */
int tree_put(struct tree_t *tree, char *key, struct data_t *value){

  if (tree == NULL || key == NULL || value == NULL)
    return -1;

  //chegou a uma folha
  if (tree->data == NULL) {
    tree->data = entry_create(strdup(key), data_dup(value));
    return 0;
  }

  int cmp = strcmp(key, tree -> data -> key);

  if (cmp == 0) {
    //se existir substitui
    entry_replace(tree -> data, strdup(key), data_dup(value));
    return 0;

  } else if (cmp > 0) {
      //se for null tem de se criar os filhos do no
      if (tree -> right == NULL)
        tree -> right = tree_create();

      return tree_put(tree -> right, key, value);

   } else {
       //se for null tem de se criar os filhos do no
      if (tree -> left == NULL)
        tree -> left = tree_create();

      return tree_put(tree -> left, key, value);
  }
}

/* Função para obter da árvore o valor associado à chave key.
 * A função deve devolver uma cópia dos dados que terão de ser
 * libertados no contexto da função que chamou tree_get, ou seja, a
 * função aloca memória para armazenar uma *CÓPIA* dos dados da árvore,
 * retorna o endereço desta memória com a cópia dos dados, assumindo-se
 * que esta memória será depois libertada pelo programa que chamou
 * a função. Devolve NULL em caso de erro.
 */
struct data_t *tree_get(struct tree_t *tree, char *key){

  if (tree == NULL || tree -> data == NULL || key == NULL)
    return NULL;

  int cmp = strcmp(key, tree -> data -> key);

  if (cmp == 0)
    return data_dup(tree -> data -> value);
  else if (cmp > 0)
    return tree_get(tree -> right, key);
  else
    return tree_get(tree -> left, key);
}

/* Função para remover um elemento da árvore, indicado pela chave key,
 * libertando toda a memória alocada na respetiva operação tree_put.
 * Retorna 0 (ok) ou -1 (key not found).
 */
int tree_del(struct tree_t *tree, char *key){

  struct data_t *data = tree_get(tree, key);

  if (tree == NULL || data == NULL)
    return -1;

  if(tree -> left == NULL && tree -> right == NULL){
    entry_destroy(tree -> data);
    tree -> data = NULL;
    tree -> left = NULL;
    tree -> right = NULL;
  }

  data_destroy(data);
  //corrige a arovre
  tree = delete_recursion(tree, key);
  return 0;
}

/*
 * Função para remover um elemento da árvore, indicado pela chave key.
 * Retorna a raiz da nova árvore corrigida
 */
struct tree_t *delete_recursion(struct tree_t *tree, char *key) {

    if (tree == NULL || tree -> data == NULL || key == NULL)
        return tree;

    int cmp = strcmp(key, tree -> data -> key);

    if (cmp == 0) {
    //valor que se tem de apagar na raiz
        //ve o numero de filhos
        if (tree -> left == NULL) {
            struct tree_t *temp = tree -> right;
            entry_destroy(tree -> data);
            free(tree);
            return temp;

        } else if (tree -> right == NULL) {
            struct tree_t *temp = tree -> left;
            entry_destroy(tree -> data);
            free(tree);
            return temp;
        }

        struct entry_t *tempTree = get_largest_child(tree -> right);
        entry_replace(tree -> data, strdup(tempTree -> key), data_dup(tempTree -> value));
        //vai corrigir o lado direito apagando a tempTree anterior
        tree -> right = delete_recursion(tree -> right, tempTree -> key);

    } else if (cmp > 0)
        tree -> right = delete_recursion(tree -> right, key);
    else
        tree -> left = delete_recursion(tree -> left, key);

    return tree;
}

/*
 * Função que devolve a entrada maior mais pequena
 */
struct entry_t *get_largest_child(struct tree_t *tree) {

  if (tree->left != NULL)
    return get_largest_child(tree->left);

  return tree->data;
}

/* Função que devolve o número de elementos contidos na árvore.
 */
int tree_size(struct tree_t *tree){

  if (tree == NULL || tree -> data == NULL)
    return 0;

  return 1 + tree_size(tree -> left) + tree_size(tree -> right);
}

/* Função que devolve a altura da árvore.
 */
int tree_height(struct tree_t *tree){

  if (tree == NULL || tree -> data == NULL)
    return 0;

  return 1 + (tree_height(tree->left) > tree_height(tree->right) ? tree_height(tree->left) : tree_height(tree->right));
}

/* Função que devolve um array de char* com a cópia de todas as keys da
 * árvore, colocando o último elemento do array com o valor NULL e
 * reservando toda a memória necessária. As keys devem vir ordenadas segundo a
 * ordenação lexicográfica das mesmas.
 */
char **tree_get_keys(struct tree_t *tree){

  int index = 0;

  if (tree_size(tree) == 0)
    return NULL;

  //apontador para saber ultima posicao
  int *pIndex;
  pIndex = &index;

  char **keys = malloc((tree_size(tree) + 1) * sizeof(char *));
  get_data_recursion(1, tree, keys, NULL, pIndex);

  keys[*pIndex] = NULL;
  return keys;
}


/* Função que devolve um array de void* com a cópia de todas os values da
 * árvore, colocando o último elemento do array com o valor NULL e
 * reservando toda a memória necessária.
 */
void **tree_get_values(struct tree_t *tree){

  int index = 0;

  //apontador para indice para saber ultima posicao
  int *pIndex;
  pIndex = &index;

  void **values = malloc((tree_size(tree) + 1) * sizeof(char *));
  get_data_recursion(0, tree, NULL, values, pIndex);
  values[*pIndex] = NULL;

  return values;
}

/*
* Funcao recursiva para ir buscar os values - INORDER
* data - serve para sabermos se sao values ou keys
* keys - se for data = 1 usar values a NULL
* values - se for data = 0 usar keys a NULL
*/
void get_data_recursion(int data, struct tree_t *tree, char** keys, void **values, int *index) {

    if (tree == NULL || tree -> data == NULL)
      return;

    get_data_recursion(data,
                       tree -> left,
                       data == 0 ? NULL : keys,
                       data == 0 ? values : NULL,
                       index);

    //Se for data == 0 queremos os values e portanto tenmos de colocar no values
    if(data == 0 )
      values[*index] = strdup(tree -> data -> value -> data);
    // se for data == 1 queremos as keys e portanto temos de colocar no keys
    else
      keys[*index] = strdup(tree -> data -> key);

    *index += 1;

    get_data_recursion(data,
                       tree -> right,
                       data == 0 ? NULL : keys,
                       data == 0 ? values : NULL,
                       index);
}


/* Função que liberta toda a memória alocada por tree_get_keys().
 */
void tree_free_keys(char **keys){

  int i = 0;

  while (keys[i] != NULL) {
    free(keys[i]);
    i++;
  }

  free(keys);
}

/* Função que liberta toda a memória alocada por tree_get_values().
 */
void tree_free_values(void **values){

  int i = 0;

  while (values[i] != NULL) {
    free(values[i]);
    i++;
  }

  free(values);
}
