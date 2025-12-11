#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#define MAX_WATCHES 8192   // Możesz zwiększyć

// Struktura przechowująca informację (wd -> path)
struct WatchEntry{
    int wd;
    char path[1024];
};
typedef struct WatchEntry WatchEntry;
WatchEntry watch_list[MAX_WATCHES];
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
        watch_list[watch_count].wd = wd;
        strncpy(watch_list[watch_count].path, path,
                sizeof(watch_list[watch_count].path) - 1);
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

void handle_event(struct inotify_event *ev)
{
    // Znajdź ścieżkę powiązaną z tym wd
    const char *path = NULL;
    for (int i = 0; i < watch_count; i++) {
        if (watch_list[i].wd == ev->wd) {
            path = watch_list[i].path;
            break;
        }
    }

    printf("\n[EVENT] w: %s\n", path ? path : "(nieznane)");

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
    if (ev->mask & IN_OPEN) {
        printf("  -> IN_OPEN:    %s\n", ev->name);
        // TODO: on_open(...)
    }
    if (ev->mask & IN_CLOSE_WRITE) {
        printf("  -> IN_CLOSE_WRITE: %s\n", ev->name);
        // TODO: on_close_write(...)
    }
    if (ev->mask & IN_CLOSE_NOWRITE) {
        printf("  -> IN_CLOSE_NOWRITE: %s\n", ev->name);
        // TODO: on_close_nowrite(...)
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
    if (ev->mask & IN_DELETE_SELF) {
        printf("  -> IN_DELETE_SELF\n");
        // TODO: on_delete_self(...)
    }
    if (ev->mask & IN_MOVE_SELF) {
        printf("  -> IN_MOVE_SELF\n");
        // TODO: on_move_self(...)
    }
    if (ev->mask & IN_UNMOUNT) {
        printf("  -> IN_UNMOUNT\n");
        // TODO: on_unmount(...)
    }
    if (ev->mask & IN_Q_OVERFLOW) {
        printf("  -> IN_Q_OVERFLOW (kolejka przepełniona!)\n");
        // TODO: on_overflow(...)
    }
}

void commit_watch(const char* path)
{
    int fd = inotify_init1(0);

    uint32_t mask =
        IN_ALL_EVENTS;

    // Dodaj obserwację na katalog i wszystkie podkatalogi
    add_watch_recursive(fd, path, mask);

    char buf[4096]
        __attribute__((aligned(__alignof__(struct inotify_event))));

    // Główna pętla
    while (1) {
        ssize_t len = read(fd, buf, sizeof(buf));
        if (len <= 0) {
            perror("read");
            continue;
        }
        char *ptr = buf;
        while (ptr < buf + len) {
            struct inotify_event *ev = (struct inotify_event *)ptr;
            handle_event(ev);
            ptr += sizeof(struct inotify_event) + ev->len;
        }
    }

    return;
}