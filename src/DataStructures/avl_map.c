#include "avl_map.h"
#include <stdlib.h>
#include <stdio.h>

// Pomocnicze funkcje
static int max(int a, int b) {
    return (a > b) ? a : b;
}

static int height(AVLNode* node) {
    return node ? node->height : 0;
}

static int balance_factor(AVLNode* node) {
    return node ? height(node->left) - height(node->right) : 0;
}

static AVLNode* create_node(int key, void* value) {
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

// Rotacje AVL
static AVLNode* rotate_right(AVLNode* y) {
    AVLNode* x = y->left;
    AVLNode* T2 = x->right;

    x->right = y;
    y->left = T2;

    y->height = max(height(y->left), height(y->right)) + 1;
    x->height = max(height(x->left), height(x->right)) + 1;

    return x;
}

static AVLNode* rotate_left(AVLNode* x) {
    AVLNode* y = x->right;
    AVLNode* T2 = y->left;

    y->left = x;
    x->right = T2;

    x->height = max(height(x->left), height(x->right)) + 1;
    y->height = max(height(y->left), height(y->right)) + 1;

    return y;
}

// Znajdź minimalny węzeł w poddrzewie
static AVLNode* find_min_node(AVLNode* node) {
    while (node && node->left) {
        node = node->left;
    }
    return node;
}

// Wstawianie (rekurencyjne)
static AVLNode* insert_node(AVLNode* node, int key, void* value, bool* inserted) {
    if (!node) {
        *inserted = true;
        return create_node(key, value);
    }

    if (key < node->key) {
        node->left = insert_node(node->left, key, value, inserted);
    } else if (key > node->key) {
        node->right = insert_node(node->right, key, value, inserted);
    } else {
        // Klucz już istnieje - aktualizacja wartości
        node->value = value;
        *inserted = false;
        return node;
    }

    // Aktualizacja wysokości
    node->height = 1 + max(height(node->left), height(node->right));

    // Balansowanie drzewa
    int balance = balance_factor(node);

    // Lewa lewa
    if (balance > 1 && key < node->left->key) {
        return rotate_right(node);
    }

    // Prawa prawa
    if (balance < -1 && key > node->right->key) {
        return rotate_left(node);
    }

    // Lewa prawa
    if (balance > 1 && key > node->left->key) {
        node->left = rotate_left(node->left);
        return rotate_right(node);
    }

    // Prawa lewa
    if (balance < -1 && key < node->right->key) {
        node->right = rotate_right(node->right);
        return rotate_left(node);
    }

    return node;
}

// Usuwanie (rekurencyjne)
static AVLNode* delete_node(AVLNode* root, int key, bool* removed) {
    if (!root) {
        *removed = false;
        return NULL;
    }

    if (key < root->key) {
        root->left = delete_node(root->left, key, removed);
    } else if (key > root->key) {
        root->right = delete_node(root->right, key, removed);
    } else {
        *removed = true;
        
        // Węzeł z jednym lub bez dzieci
        if (!root->left || !root->right) {
            AVLNode* temp = root->left ? root->left : root->right;
            
            if (!temp) {
                temp = root;
                root = NULL;
            } else {
                *root = *temp;
            }
            
            free(temp);
        } else {
            // Węzeł z dwoma dziećmi
            AVLNode* temp = find_min_node(root->right);
            root->key = temp->key;
            root->value = temp->value;
            root->right = delete_node(root->right, temp->key, removed);
        }
    }

    if (!root) return NULL;

    // Aktualizacja wysokości
    root->height = 1 + max(height(root->left), height(root->right));

    // Balansowanie drzewa
    int balance = balance_factor(root);

    // Lewa lewa
    if (balance > 1 && balance_factor(root->left) >= 0) {
        return rotate_right(root);
    }

    // Lewa prawa
    if (balance > 1 && balance_factor(root->left) < 0) {
        root->left = rotate_left(root->left);
        return rotate_right(root);
    }

    // Prawa prawa
    if (balance < -1 && balance_factor(root->right) <= 0) {
        return rotate_left(root);
    }

    // Prawa lewa
    if (balance < -1 && balance_factor(root->right) > 0) {
        root->right = rotate_right(root->right);
        return rotate_left(root);
    }

    return root;
}

// Wyszukiwanie (rekurencyjne)
static AVLNode* search_node(AVLNode* node, int key) {
    if (!node || node->key == key) {
        return node;
    }
    
    if (key < node->key) {
        return search_node(node->left, key);
    }
    
    return search_node(node->right, key);
}

// Przechodzenie inorder (rekurencyjne)
static void inorder_traversal_node(AVLNode* node, void (*func)(int, void*)) {
    if (node) {
        inorder_traversal_node(node->left, func);
        func(node->key, node->value);
        inorder_traversal_node(node->right, func);
    }
}

// Usuwanie wszystkich węzłów (rekurencyjne)
static void clear_nodes(AVLNode* node) {
    if (node) {
        clear_nodes(node->left);
        clear_nodes(node->right);
        free(node);
    }
}

// Implementacja interfejsu publicznego

AVLMap* avl_map_create() {
    AVLMap* map = (AVLMap*)malloc(sizeof(AVLMap));
    if (map) {
        map->root = NULL;
        map->size = 0;
    }
    return map;
}

void avl_map_destroy(AVLMap* map) {
    if (map) {
        avl_map_clear(map);
        free(map);
    }
}

void avl_map_clear(AVLMap* map) {
    if (map) {
        clear_nodes(map->root);
        map->root = NULL;
        map->size = 0;
    }
}

bool avl_map_insert(AVLMap* map, int key, void* value) {
    if (!map) return false;
    
    bool inserted = false;
    map->root = insert_node(map->root, key, value, &inserted);
    
    if (inserted) {
        map->size++;
    }
    
    return inserted;
}

void* avl_map_get(AVLMap* map, int key) {
    if (!map) return NULL;
    
    AVLNode* node = search_node(map->root, key);
    return node ? node->value : NULL;
}

bool avl_map_remove(AVLMap* map, int key) {
    if (!map) return false;
    
    bool removed = false;
    map->root = delete_node(map->root, key, &removed);
    
    if (removed) {
        map->size--;
    }
    
    return removed;
}

bool avl_map_contains(AVLMap* map, int key) {
    return map && search_node(map->root, key) != NULL;
}

int avl_map_size(AVLMap* map) {
    return map ? map->size : 0;
}

bool avl_map_is_empty(AVLMap* map) {
    return map ? map->size == 0 : true;
}

void avl_map_inorder_traversal(AVLMap* map, void (*func)(int, void*)) {
    if (map && func) {
        inorder_traversal_node(map->root, func);
    }
}

int avl_map_min_key(AVLMap* map) {
    if (!map || !map->root) {
        return -1; // lub inna wartość oznaczająca błąd
    }
    
    AVLNode* current = map->root;
    while (current->left) {
        current = current->left;
    }
    
    return current->key;
}

int avl_map_max_key(AVLMap* map) {
    if (!map || !map->root) {
        return -1; // lub inna wartość oznaczająca błąd
    }
    
    AVLNode* current = map->root;
    while (current->right) {
        current = current->right;
    }
    
    return current->key;
}