#ifndef AVL_MAP_H
#define AVL_MAP_H

#include <stddef.h>

// Typy dla klucza i wartości (wskaźniki na dowolny typ)
typedef void* Key;
typedef void* Value;

// Struktura węzła AVL
typedef struct AVLNode {
    Key key;
    Value value;
    struct AVLNode* left;
    struct AVLNode* right;
    int height;
} AVLNode;

// Struktura mapy AVL
typedef struct {
    AVLNode* root;
    size_t size;
    
    // Funkcje dostarczane przez użytkownika
    int (*compare_keys)(Key, Key);          // porównywanie kluczy
    void (*free_key_value)(Key, Value);    // dealokacja klucza i wartości
} AVLMap;

// Inicjalizacja mapy
AVLMap* avl_map_create(
    int (*compare_keys)(Key, Key),
    void (*free_key_value)(Key, Value)
);

// Usunięcie mapy i zwolnienie pamięci
void avl_map_destroy(AVLMap* map);

// Wstawienie pary klucz-wartość
int avl_map_insert(AVLMap* map, Key key, Value value);

// Usunięcie wartości na podstawie klucza
int avl_map_remove(AVLMap* map, Key key);

// Wyszukanie wartości na podstawie klucza
Value avl_map_find(AVLMap* map, Key key);

// Sprawdzenie czy klucz istnieje
int avl_map_contains(AVLMap* map, Key key);

// Rozmiar mapy
size_t avl_map_size(AVLMap* map);

// Czy mapa jest pusta
int avl_map_is_empty(AVLMap* map);

// Iteracja po wszystkich elementach
void avl_map_foreach(AVLMap* map, void (*callback)(Key, Value, void*), void* user_data);

#endif // AVL_MAP_H