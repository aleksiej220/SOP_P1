#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h> 
#include "../DataStructures/avl_map.h"
#include "copying.h"
#include "controlPanel.h"

#define MAX_WATCHES INT32_MAX   // Możesz zwiększyć
#define MAX_DIGITS 20
// struktura przechowujaca wd->path
//zmienne procesowe globalne, ale ustawiane tylko dla tego procesu
AVLMap *watch_list;


void clean_each(int key, void* value) {
    free(value);
}

int watch_count = 0;
/* ---------------------------------------------------------
   Dodaj WATCH i zapamiętaj go
--------------------------------------------------------- */
int add_watch(int fd, const char *path, uint32_t mask)
{
    int wd = inotify_add_watch(fd, path, mask);
    if (wd == -1) {
        fprintf(stderr, "Nie można dodać watcha dla %s: %s\n",
                path, strerror(errno));
        return -1;
    }

    if (watch_count < MAX_WATCHES) {
        avl_map_insert(watch_list,wd,strdup(path));
        watch_count++;
    }

    printf("[WATCH] %s (wd=%d)\n", path, wd);
    return wd;
}

/* ---------------------------------------------------------
   Rekursyjne dodawanie watcherów na katalog i podkatalogi
--------------------------------------------------------- */
void add_watch_recursive(int fd, const char *dirpath, uint32_t mask)
{
    // Najpierw dodaj watcher dla katalogu
    add_watch(fd, dirpath, mask);

    DIR *dir = opendir(dirpath);
    if (!dir) return;

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {

        // Ignorujemy . i ..
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s",
                 dirpath, entry->d_name);

        struct stat st;
        if (stat(fullpath, &st) == -1)
            continue;

        // Jeśli to katalog — dodajemy rekurencyjnie
        if (S_ISDIR(st.st_mode)) {
            add_watch_recursive(fd, fullpath, mask);
        }
    }

    closedir(dir);
}

void handle_event(struct inotify_event* ev, const copy_info* info)
{
    // Znajdź ścieżkę powiązaną z tym wd
    char *path = NULL;
    if(!avl_map_contains(watch_list,ev->wd)){
        printf("EVENTA %d NIE MA W LISCIE\n",ev->wd);
    }
    path = avl_map_get(watch_list,ev->wd);
    //printf("\n[EVENT] w: %s\n", path ? path : "(nieznane)");

    /* -------------------------------------------
       TU WSTAWIASZ WŁASNĄ REAKCJĘ NA ZDARZENIA
       ------------------------------------------- */

    if (ev->mask & IN_CREATE) {
        printf("  -> IN_CREATE:  %s\n", ev->name);  
        // TODO: twoja funkcja on_create(...)
    }
    if (ev->mask & IN_DELETE) {
        printf("  -> IN_DELETE:  %s\n", ev->name);
        // TODO: on_delete(...)
    }
    if (ev->mask & IN_MODIFY) {
        printf("  -> IN_MODIFY:  %s\n", ev->name);
        // TODO: on_modify(...)
    }
    if (ev->mask & IN_ATTRIB) {
        printf("  -> IN_ATTRIB:  %s\n", ev->name);
        // TODO: on_attrib_change(...)
    }
    if (ev->mask & IN_MOVED_FROM) {
        printf("  -> IN_MOVED_FROM: %s (cookie=%u)\n",
               ev->name, ev->cookie);
        // TODO: on_moved_from(...)
    }
    if (ev->mask & IN_MOVED_TO) {
        printf("  -> IN_MOVED_TO:   %s (cookie=%u)\n",
               ev->name, ev->cookie);
        // TODO: on_moved_to(...)
    }
}
controlPanel* initialize_watch(copy_info* info){

    watch_list = avl_map_create();

    //wspoldzielony panel sterowania
    controlPanel *panel = mmap(NULL, sizeof(controlPanel),PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS,-1, 0);
    panel->watch = 1;
    pid_t pid = fork();
    if(pid<0){
        //blad
        printf("BLAD FORKOWANIA\n");
        return panel;
    }
    else if(pid == 0){
        // kod dziecka
        int fd = inotify_init1(0);
        info->fd = fd;
        uint32_t mask =
            IN_ALL_EVENTS;

        int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);

        // Dodaj obserwację
        add_watch_recursive(fd, info->root_src, mask);

        char buf[4096]
            __attribute__((aligned(__alignof__(struct inotify_event))));

        printf("Started\n");
        int iterations = 0;
        while (panel->watch) {
            iterations++;
            ssize_t len = read(fd, buf, sizeof(buf));
            if (len < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // brak zdarzeń — NIE blokuje programu
                    sleep(0.01); // opcjonalnie: 10 ms, żeby nie mielić CPU
                    continue;
                } else {
                    perror("read");
                    break;
                }
            }

            char *ptr = buf;
            while (ptr < buf + len) {
                struct inotify_event *ev = (struct inotify_event *)ptr;
                handle_event(ev, info);
                ptr += sizeof(struct inotify_event) + ev->len;
            }
        }
        printf("Iterations: %i\n",iterations);
        avl_map_inorder_traversal(watch_list,clean_each);

        avl_map_destroy(watch_list);
        exit(0);
        return panel;
    }
    else{
        //kod rodzica
        return panel;
    }
}