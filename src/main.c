
/*
    Autor kodu: Alex Siurnicki
    Indeks: 339092
*/

#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#include "Commands/copying.h"
#include "Commands/watcher.h"
#include "DataStructures/avl_map.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#define MAX_LINE 1024
#define MAX_TOKENS 64

int compare_keys(Key a, Key b){
    return strcmp((char*)a,(char*)b);
}

void clean_node(Key key, Value val){
    free(key);
    // `val` points to a `controlPanel` allocated with `mmap` in `initialize_watch`,
    // so unmap it instead of calling `free`.
    munmap(val, sizeof(*(controlPanel*)val));
}
void terminator(Key key, Value val, void* inf){
    controlPanel* panel = (controlPanel*)val;
    panel->terminate = 1;
    while (panel->terminate)
    {
        sleep(0.01);
    }
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
void write_commands(){
    printf("----------------\n");
    printf("| LISTA KOMEND |\n");
    printf("----------------\n");
    printf("help - pokazuje liste dostępnych komend\n");
    printf("add <source path> <target path1> <target path2> ... - tworzy kopie zapasowe i procesy od source do targetów\n");
    printf("end <source path> <target path1> <target path2> ... - przerywa tworzenie kopii zapasowej\n");
    printf("restore <source path> <target path> - przywraca kopię zapasową kończąc proces\n");
    printf("list  - zwraca listę procesów\n");
    printf("exit  - kończy działanie programu\n");
    printf("\n");
}
int main(void)
{
    AVLMap* copies = avl_map_create(compare_keys, clean_node);
    char line[MAX_LINE];
    char *tokens[MAX_TOKENS];
    int token_count = 0;
    write_commands();
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

                printf("Obsługa '%s' do '%s':\n",source_path, target_path);
                copy_info info;
                char * op1 = realpath(source_path,info.root_src);
                char * op2 = realpath(target_path,info.root_dst);
                //
                DIR *dir;
                dir = opendir(source_path);
                if(!dir || op1==NULL){
                    printf("Katalog %s nie istnieje\n",source_path);
                    continue;
                }
                if(!op2){
                    if (mkdir(target_path, 0777) < 0 && errno != EEXIST) {
                        printf("Niepoprawny adres: %s\n",target_path);
                        continue;
                    }
                    op2 = realpath(target_path,info.root_dst);
                }
                else{
                    /* zamykamy uchwyt do katalogu źródłowego przed otwarciem katalogu docelowego,
                       żeby nie tracić referencji i nie doprowadzać do wycieku */
                    closedir(dir);
                    dir = opendir(target_path);
                    if(!dir){
                        printf("%s to nie katalog\n",target_path);
                        continue;
                    }
                    else{
                        if(!is_dir_empty(dir)){
                            printf("Katalog %s nie jest pusty\n",target_path);
                            closedir(dir);
                            continue;
                        }
                    }
                    /* nie potrzebujemy już uchwytu do katalogu docelowego */
                    closedir(dir);
                }
                char str[PATH_MAX];
                connect(info.root_src,info.root_dst,str);

                if(avl_map_contains(copies,str)){
                    printf("Proces dla: %s do %s już istnieje. Zostanie pominięty!\n",source_path, target_path);
                    continue;
                }
                controlPanel * panel = initialize_watch(info);
                avl_map_insert(copies,strdup(str),panel);
                printf("Pomyślnie utworzono proces dla: %s do %s\n",info.root_src, info.root_dst);
            }
        }
        else if(strcmp(tokens[0], "help") == 0){
            if (token_count > 1) {
                printf("Błąd: za dużo argumentów\n");
                continue;
            }
            write_commands();
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
            if (token_count < 3) {
                printf("Błąd: za mało argumentów\n");
                continue;
            }
            for (int i = 2; i < token_count; i++) {
                char *source = tokens[1];
                char *dest = tokens[i];
                char rsource[PATH_MAX];
                char rdest[PATH_MAX];
                char * op1 = realpath(source,rsource);
                char * op2 = realpath(dest,rdest);
                if((!op1) || (!op2)){
                    printf("Proces dla: %s do %s nie istnieje. Wskaż istniejący proces\n",source, dest);
                    continue;
                }
                char str[PATH_MAX];
                connect(rsource,rdest,str);
                if(!avl_map_contains(copies,str)){
                        printf("Proces dla: %s do %s nie istnieje. Wskaż istniejący proces\n",source, dest);
                        continue;
                }
                controlPanel * panel = avl_map_find(copies,str);
                panel->watch = 0;
                printf("Proces dla: %s do %s zakończył obserwację\n",source, dest);
            }
        }
        else if(strcmp(tokens[0], "restore") == 0){
            if (token_count != 3) {
                printf("Błąd: nieodpowiendnia liczba argumentów\n");
                continue;
            }
            char *source = tokens[1];
            char *dest = tokens[2];
            char rsource[PATH_MAX];
            char rdest[PATH_MAX];
            char * op1 = realpath(source,rsource);
            char * op2 = realpath(dest,rdest);
            if((!op1) || (!op2)){
                printf("Proces dla: %s do %s nie istnieje. Wskaż istniejący proces\n",source, dest);
                continue;
            }
            char str[PATH_MAX];
            connect(rsource,rdest,str);
            if(!avl_map_contains(copies,str)){
                    printf("Proces dla: %s do %s nie istnieje. Wskaż istniejący proces\n",source, dest);
                    continue;
            }
            controlPanel * panel = avl_map_find(copies,str);
            printf("Przywracanie...\n");
            panel->watch = 0;
            panel->restore = 1;
            while (panel->restore)
            {
                sleep(0.01);
            }
            avl_map_remove(copies,str);
            printf("Przywrócono pomyślnie\n");
        }
        else if(strcmp(tokens[0], "exit") == 0){
            if (token_count > 1) {
                printf("Błąd: za dużo argumentów\n");
                continue;
            }
            //tutaj zabijamy cały program
            avl_map_foreach(copies,terminator,NULL);
            avl_map_destroy(copies);
            exit(0);
        }
        else{
            printf("Nieznane polecenie: %s\n", tokens[0]);
            continue;
        }
    }
}