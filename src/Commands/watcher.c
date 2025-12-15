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
#define MAX_DIGITS 20 //mak
// struktura przechowujaca wd->path
//zmienne procesowe globalne, ale ustawiane tylko dla tego procesu
AVLMap *watch_list;

void clean(Key key, Value value) {
    printf("CLEANING %i,%s\n",*(int*)key,(char*) value);
    free(key);
    free(value);
}
int compare(Key a,Key b){
    int* p1 = (int*)a;
    int* p2 = (int*)b;
    return *p1 - *p2;
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
    int* key = malloc(sizeof(int));
    *key = wd;
    if (watch_count < MAX_WATCHES) {
        avl_map_insert(watch_list,key,strdup(path));
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

void handle_event(struct inotify_event* ev, const copy_info* info,uint32_t mask)
{

    char src[4096];
    char dst[4096];
    // Znajdź ścieżkę powiązaną z tym wd
    if(!avl_map_contains(watch_list,&(ev->wd))){
        return;
        //printf("EVENTA %d NIE MA W LISCIE\n",ev->wd);
    }
    char * path = (char*)avl_map_find(watch_list,&(ev->wd));

    sprintf(src,"%s/%s",path,ev->name);
    convert_path(src,dst,info);

    //printf("\n[EVENT] w: %s\n", path ? path : "(nieznane)");

    /* ----------------------
       REAKCJA NA ZDARZENIA
       ---------------------- */

    if (ev->mask & (IN_CREATE | IN_MODIFY | IN_ATTRIB | IN_MOVED_TO)) {
        printf("COPYING:  %s to %s\n", src, dst); 
        copy_entry(src,dst,info);
        if(ev->mask & IN_ISDIR){
            add_watch_recursive(info->fd,src,mask);
        }
    }
    if (ev->mask & (IN_DELETE | IN_MOVED_FROM)) {
        printf("DELETING:  %s\n", ev->name);
        remove_recursive(dst);
    }
    if (ev->mask & (IN_DELETE_SELF | IN_MOVE_SELF)){
        printf("Removing watch %i\n",ev->wd);
        avl_map_remove(watch_list,&(ev->wd));
    }
}
controlPanel* initialize_watch(copy_info info){

    watch_list = avl_map_create(compare,clean);

    //wspoldzielony panel sterowania
    controlPanel *panel = mmap(NULL, sizeof(controlPanel),PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANONYMOUS,-1, 0);
    panel->watch = 1;
    panel->terminate = 0;
    panel->restore = 0;
    pid_t pid = fork();
    if(pid<0){
        //blad
        printf("BLAD FORKOWANIA\n");
        return panel;
    }
    else if(pid == 0){
        // kod dziecka

        //kopiowanie

        copy_entry(info.root_src,info.root_dst,&info);

        //watcher
        int fd = inotify_init1(0);
        info.fd = fd;
        uint32_t mask =
            IN_ALL_EVENTS;

        int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);

        // Dodaj obserwację
        add_watch_recursive(fd, info.root_src, mask);

        char buf[4096]
            __attribute__((aligned(__alignof__(struct inotify_event))));

        printf("Started\n");
        int iterations = 0;
        while (1) {
            iterations++;
            if(panel->watch){
                ssize_t len = read(fd, buf, sizeof(buf));
                if (len < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        // brak zdarzeń — NIE blokuje programu
                    } else {
                        perror("read");
                        break;
                    }
                }
                else{
                    char *ptr = buf;
                    while (ptr < buf + len) {
                        struct inotify_event *ev = (struct inotify_event *)ptr;
                        handle_event(ev, &info,mask);
                        ptr += sizeof(struct inotify_event) + ev->len;
                    }
                }
            }
            if(panel->terminate){
                printf("Terminating\n");
                break;
            }
            if(panel->restore){
                remove_recursive(info.root_src);
                copy_info info2;
                strcpy(info2.root_src , info.root_dst);
                strcpy(info2.root_dst , info.root_src);
                copy_entry(info2.root_src,info2.root_dst,&info2);
                panel->restore = 0;
                break;
            }
            sleep(0.01); // opcjonalnie: 10 ms, żeby nie mielić CPU
        }
        //printf("Iterations: %i\n",iterations);
        close(fd);
        avl_map_destroy(watch_list);
        panel->terminate = 0;
        exit(0);
        return panel;
    }
    else{
        //kod rodzica
        return panel;
    }
}