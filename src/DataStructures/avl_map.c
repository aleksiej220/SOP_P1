#include "avl_map.h"
#include <stdlib.h>

// Funkcje pomocnicze

// Obliczanie wysokości węzła
static int height(AVLNode* node) {
    return node ? node->height : 0;
}

// Obliczanie współczynnika balansu
static int balance_factor(AVLNode* node) {
    return node ? height(node->left) - height(node->right) : 0;
}

// Aktualizacja wysokości węzła
static void update_height(AVLNode* node) {
    if (node) {
        int left_height = height(node->left);
        int right_height = height(node->right);
        node->height = (left_height > right_height ? left_height : right_height) + 1;
    }
}

// Rotacja w prawo
static AVLNode* rotate_right(AVLNode* y) {
    AVLNode* x = y->left;
    AVLNode* T2 = x->right;
    
    x->right = y;
    y->left = T2;
    
    update_height(y);
    update_height(x);
    
    return x;
}

// Rotacja w lewo
static AVLNode* rotate_left(AVLNode* x) {
    AVLNode* y = x->right;
    AVLNode* T2 = y->left;
    
    y->left = x;
    x->right = T2;
    
    update_height(x);
    update_height(y);
    
    return y;
}

// Balansowanie węzła
static AVLNode* balance_node(AVLNode* node) {
    if (!node) return NULL;
    
    update_height(node);
    int bf = balance_factor(node);
    
    // Lewa lewa
    if (bf > 1 && balance_factor(node->left) >= 0) {
        return rotate_right(node);
    }
    
    // Lewa prawa
    if (bf > 1 && balance_factor(node->left) < 0) {
        node->left = rotate_left(node->left);
        return rotate_right(node);
    }
    
    // Prawa prawa
    if (bf < -1 && balance_factor(node->right) <= 0) {
        return rotate_left(node);
    }
    
    // Prawa lewa
    if (bf < -1 && balance_factor(node->right) > 0) {
        node->right = rotate_right(node->right);
        return rotate_left(node);
    }
    
    return node;
}

// Znajdowanie minimalnego węzła
static AVLNode* find_min(AVLNode* node) {
    while (node && node->left) {
        node = node->left;
    }
    return node;
}

// Tworzenie nowego węzła
static AVLNode* create_node(Key key, Value value) {
    AVLNode* node = (AVLNode*)malloc(sizeof(AVLNode));
    if (node) {
        node->key = key;
        node->value = value;
        node->left = NULL;
        node->right = NULL;
        node->height = 1;
    }
    return node;
}

// Rekurencyjne wstawianie
static AVLNode* insert_node(AVLNode* node, Key key, Value value, int (*compare)(Key, Key), int* inserted) {
    if (!node) {
        *inserted = 1;
        return create_node(key, value);
    }
    
    int cmp = compare(key, node->key);
    
    if (cmp < 0) {
        node->left = insert_node(node->left, key, value, compare, inserted);
    } else if (cmp > 0) {
        node->right = insert_node(node->right, key, value, compare, inserted);
    } else {
        // Klucz już istnieje
        *inserted = 0;
        return node;
    }
    
    return balance_node(node);
}

// Rekurencyjne usuwanie
static AVLNode* remove_node(AVLNode* node, Key key, int (*compare)(Key, Key), 
                          void (*free_key_value)(Key, Value), int* removed) {
    if (!node) {
        *removed = 0;
        return NULL;
    }
    
    int cmp = compare(key, node->key);
    
    if (cmp < 0) {
        node->left = remove_node(node->left, key, compare, free_key_value, removed);
    } else if (cmp > 0) {
        node->right = remove_node(node->right, key, compare, free_key_value, removed);
    } else {
        // Znaleziono węzeł do usunięcia
        *removed = 1;
        
        // Jeśli węzeł ma 0 lub 1 dziecko
        if (!node->left || !node->right) {
            AVLNode* temp = node->left ? node->left : node->right;
            
            if (temp) {
                // Jedno dziecko
                *node = *temp;
                free(temp);
            } else {
                // Brak dzieci
                if (free_key_value) {
                    free_key_value(node->key, node->value);
                }
                free(node);
                return NULL;
            }
        } else {
            // Dwoje dzieci - znajdź następnika inorder
            AVLNode* temp = find_min(node->right);
            
            // Zachowaj stare klucze do dealokacji
            Key old_key = node->key;
            Value old_value = node->value;
            
            // Skopiuj dane następnika
            node->key = temp->key;
            node->value = temp->value;
            
            // Usuń następnika
            node->right = remove_node(node->right, temp->key, compare, NULL, removed);
            
            // Dealokuj stare dane
            if (free_key_value) {
                free_key_value(old_key, old_value);
            }
        }
    }
    
    return balance_node(node);
}

// Rekurencyjne wyszukiwanie
static AVLNode* find_node(AVLNode* node, Key key, int (*compare)(Key, Key)) {
    while (node) {
        int cmp = compare(key, node->key);
        if (cmp == 0) {
            return node;
        } else if (cmp < 0) {
            node = node->left;
        } else {
            node = node->right;
        }
    }
    return NULL;
}

// Rekurencyjne czyszczenie
static void clear_tree(AVLNode* node, void (*free_key_value)(Key, Value)) {
    if (node) {
        clear_tree(node->left, free_key_value);
        clear_tree(node->right, free_key_value);
        if (free_key_value) {
            free_key_value(node->key, node->value);
        }
        free(node);
    }
}

// Rekurencyjna iteracja
static void inorder_foreach(AVLNode* node, void (*callback)(Key, Value, void*), void* user_data) {
    if (node) {
        inorder_foreach(node->left, callback, user_data);
        callback(node->key, node->value, user_data);
        inorder_foreach(node->right, callback, user_data);
    }
}

// Implementacja interfejsu publicznego

AVLMap* avl_map_create(int (*compare_keys)(Key, Key), void (*free_key_value)(Key, Value)) {
    AVLMap* map = (AVLMap*)malloc(sizeof(AVLMap));
    if (map) {
        map->root = NULL;
        map->size = 0;
        map->compare_keys = compare_keys;
        map->free_key_value = free_key_value;
    }
    return map;
}

void avl_map_destroy(AVLMap* map) {
    if (map) {
        clear_tree(map->root, map->free_key_value);
        free(map);
    }
}

int avl_map_insert(AVLMap* map, Key key, Value value) {
    if (!map) return 0;
    
    int inserted = 0;
    map->root = insert_node(map->root, key, value, map->compare_keys, &inserted);
    
    if (inserted) {
        map->size++;
    }
    
    return inserted;
}

int avl_map_remove(AVLMap* map, Key key) {
    if (!map) return 0;
    
    int removed = 0;
    map->root = remove_node(map->root, key, map->compare_keys, map->free_key_value, &removed);
    
    if (removed) {
        map->size--;
    }
    
    return removed;
}

Value avl_map_find(AVLMap* map, Key key) {
    if (!map) return NULL;
    
    AVLNode* node = find_node(map->root, key, map->compare_keys);
    return node ? node->value : NULL;
}

int avl_map_contains(AVLMap* map, Key key) {
    return avl_map_find(map, key) != NULL;
}

size_t avl_map_size(AVLMap* map) {
    return map ? map->size : 0;
}

int avl_map_is_empty(AVLMap* map) {
    return map ? map->size == 0 : 1;
}

void avl_map_foreach(AVLMap* map, void (*callback)(Key, Value, void*), void* user_data) {
    if (map && callback) {
        inorder_foreach(map->root, callback, user_data);
    }
}