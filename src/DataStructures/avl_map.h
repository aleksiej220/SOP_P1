#ifndef AVL_MAP_H
#define AVL_MAP_H

#include <stdbool.h>

// Struktura węzła AVL
typedef struct AVLNode {
    char* key;
    void* value;
    struct AVLNode* left;
    struct AVLNode* right;
    int height;
} AVLNode;

// Struktura mapy AVL
typedef struct {
    AVLNode* root;
    int size;
} AVLMap;

// Operacje na mapie
AVLMap* avl_map_create();
void avl_map_destroy(AVLMap* map);
void avl_map_clear(AVLMap* map);

// Podstawowe operacje
bool avl_map_insert(AVLMap* map, char* key, void* value);
void* avl_map_get(AVLMap* map, char* key);
bool avl_map_remove(AVLMap* map, char* key);
bool avl_map_contains(AVLMap* map, char* key);
int avl_map_size(AVLMap* map);
bool avl_map_is_empty(AVLMap* map);

// Iteracja
void avl_map_inorder_traversal(AVLMap* map, void (*func)(char*, void*));

// Operacje min/max
char* avl_map_min_key(AVLMap* map);
char* avl_map_max_key(AVLMap* map);

#endif