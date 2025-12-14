#include "Commands/copying.h"
#include "Commands/watcher.h"
#include "DataStructures/avl_map.h"
#include <stdio.h>
#include <string.h>

#define MAX_LINE 1024
#define MAX_TOKENS 64
int main(void)
{
    char line[MAX_LINE];
    char *tokens[MAX_TOKENS];
    int token_count = 0;
    while(true){
        printf("Podaj polecenie:\n");

        if (!fgets(line, sizeof(line), stdin)) {
            return 1;
        }

        // usunięcie znaku nowej linii
        line[strcspn(line, "\n")] = 0;

        // tokenizacja
        char *token = strtok(line, " ");
        while (token && token_count < MAX_TOKENS) {
            tokens[token_count++] = token;
            token = strtok(NULL, " ");
        }
        
        if (token_count < 3) {
            printf("Błąd: za mało argumentów\n");
            return 1;
        }
        // sprawdzamy komendę
        if (strcmp(tokens[0], "add") == 0) {
            char *source_path = tokens[1];

            // wszystkie kolejne to ścieżki docelowe
            for (int i = 2; i < token_count; i++) {
                char *target_path = tokens[i];

                printf("Kopiowanie z '%s' do '%s'\n",
                    source_path, target_path);

                // tutaj wołasz swoją funkcję
                // copy_directory(source_path, target_path);
            }
        }
        else{
            printf("Nieznane polecenie: %s\n", tokens[0]);
            return 1;
        }
    }
    return 0;
}