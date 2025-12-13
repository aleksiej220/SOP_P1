#ifndef COPYING_H
#define COPYING_H

#include <limits.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// Informacje potrzebne operacjom kopiowania
struct copy_info {
    int fd;                // deskryptor inotify (opcjonalny)
    char* root_src;
    char* root_dst;
};

typedef struct copy_info copy_info;

// Utils
int get_real_symlink_path(const char *src, char* path);
int is_descendant_of(const char *A, const char *B);

// Kopiowanie plików/entry
int copy_file(const char *src, const char *dst, const copy_info* info);
int copy_symlink(const char *src, const char *dst, const copy_info* info);
int copy_directory(const char *src, const char *dst, const copy_info* info);
int copy_entry(const char *src_path, const char *dst_path, const copy_info* info);

#ifdef __cplusplus
}
#endif

#endif // COPYING_H
