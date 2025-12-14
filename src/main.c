#define _POSIX_C_SOURCE 200809L

#include "Commands/copying.h"
#include "Commands/watcher.h"
#include "DataStructures/avl_map.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define MAX_LINE 1024
#define MAX_TOKENS 64

int compare_keys(Key a, Key b){
    return strcmp((char*)a,(char*)b);
}

void clean_node(Key key, Value val){
    free(key);
    free(val);
    //tutaj dopracuj free(val)
}
int connect(const char* a, const char* b, char* wynik) {
    size_t len = strlen(a) + strlen(b) + 2; // 1 na ', 1 na \0
    if (!wynik) return 0;
    snprintf(wynik, len, "%s'%s", a, b);
    return 1;
}
int disconnect(const char* input, char* a, char* b) {
    const char* sep = strchr(input, '\'');
    if (!sep) return 0; // brak separatora

    if (!a || !b) return 0;
    size_t lenA = sep - input;
    strncpy(a, input,lenA);
    a[lenA] = '\0';
    strcpy(b, sep + 1);

    return 1;
}
void list_iteration(Key key, Value val, Value misc){
    char str1[100];
    char str2[100];
    disconnect(key,str1,str2);
    printf("<%s> do <%s>\n",str1,str2);
}
int tokenize_with_quotes(char *line, char *tokens[], int max_tokens) {
    int count = 0;
    char *p = line;

    while (*p && count < max_tokens) {
        // pomiń spacje
        while (*p == ' ') p++;

        if (*p == '\0') break;

        // jeśli token w cudzysłowie
        if (*p == '"') {
            p++; // pomijamy "
            tokens[count++] = p;

            while (*p && *p != '"') p++;

            if (*p == '"') {
                *p = '\0';
                p++;
            }
        } 
        // zwykły token
        else {
            tokens[count++] = p;

            while (*p && *p != ' ') p++;

            if (*p) {
                *p = '\0';
                p++;
            }
        }
    }

    return count;
}
int main(void)
{
    AVLMap* copies = avl_map_create(compare_keys, clean_node);
    char line[MAX_LINE];
    char *tokens[MAX_TOKENS];
    int token_count = 0;
    while(1){
        printf("Podaj polecenie:\n");

        if (!fgets(line, sizeof(line), stdin)) {
            return 1;
        }

        // usunięcie znaku nowej linii
        line[strcspn(line, "\n")] = 0;

        // tokenizacja
        token_count = tokenize_with_quotes(line, tokens, MAX_TOKENS);
        
        // sprawdzamy komendę
        if (strcmp(tokens[0], "add") == 0) {
            if (token_count < 3) {
                printf("Błąd: za mało argumentów\n");
                continue;
            }
            char *source_path = tokens[1];

            // wszystkie kolejne to ścieżki docelowe
            for (int i = 2; i < token_count; i++) {
                char *target_path = tokens[i];

                printf("Obsługa '%s' do '%s'\n",source_path, target_path);
                copy_info info;
                strcpy(info.root_src,source_path);
                strcpy(info.root_dst,target_path);
                char str[100];
                connect(source_path,target_path,str);

                if(avl_map_contains(copies,str)){
                    printf("Proces dla: %s do %s już istnieje. Zostanie pominięty!\n",source_path, target_path);
                    continue;
                }
                controlPanel * panel = initialize_watch(info);
                avl_map_insert(copies,strdup(str),panel);
                // tutaj wołasz swoją funkcję
            }
        }
        else if(strcmp(tokens[0], "list") == 0){
            if (token_count > 1) {
                printf("Błąd: za dużo argumentów\n");
                continue;
            }
            printf("Oto lista procesów:\n");
            avl_map_foreach(copies,list_iteration,NULL);
        }
        else if(strcmp(tokens[0], "end") == 0){
            if (token_count != 3) {
                printf("Błąd: nieodpowiendnia liczba argumentów\n");
                continue;
            }
            char *source = tokens[1];
            char *dest = tokens[2];
            char str[100];
            connect(source,dest,str);
            if(!avl_map_contains(copies,str)){
                    printf("Proces dla: %s do %s nie istnieje. Wskaż istniejący proces\n",source, dest);
                    continue;
            }
            controlPanel * panel = avl_map_find(copies,str);
            panel->terminate = 1;
        }
        else{
            printf("Nieznane polecenie: %s\n", tokens[0]);
            continue;
        }
    }
}