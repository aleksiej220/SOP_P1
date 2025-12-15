#ifndef COPYING_H
#define COPYING_H
#define PATH_LEN 4096
#include <limits.h>
#include <sys/types.h>
#include <dirent.h>

#ifdef __cplusplus
extern "C" {
#endif

// Informacje potrzebne operacjom kopiowania
struct copy_info {  
    int fd;             // deskryptor inotify (opcjonalny)
    char root_src[PATH_LEN];
    char root_dst[PATH_LEN];
};
typedef struct copy_info copy_info;

// Utils
int is_dir_empty(DIR *dir);
int convert_path(const char * src, char * dst, const copy_info * info);
int get_real_symlink_path(const char *src, char* path);
int is_descendant_of(const char *A, const char *B);

// Kopiowanie plików/entry
int copy_file(const char *src, const char *dst, const copy_info* info);
int copy_symlink(const char *src, const char *dst, const copy_info* info);
int copy_directory(const char *src, const char *dst, const copy_info* info);
int copy_entry(const char *src_path, const char *dst_path, const copy_info* info);
int remove_recursive(const char *path);

#ifdef __cplusplus
}
#endif

#endif // COPYING_H
