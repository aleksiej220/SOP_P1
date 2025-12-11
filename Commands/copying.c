#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>
#define PATH_MAX 4096

struct copy_info{
    char* root_src; 
    char* root_dst;
    int id;
};
typedef struct copy_info copy_info;


int get_real_symlink_path(const char *src, char* path){

    //Najpierw wydobądźmy dyrektorium
    char symlink_copy[PATH_MAX];
    strncpy(symlink_copy, src, sizeof(symlink_copy));
    char* dir = dirname(symlink_copy);

    //Potem wyczytajmy zawartość 
    char target[PATH_MAX];
    ssize_t len = readlink(src, target, sizeof(target) - 1);
    if (len < 0) return -1;
    target[len] = '\0';

    // Trzeba zapisać poprzednie dyrektorium przed zmianą
    char pwd[4096];
    if (getcwd(pwd,sizeof(pwd)) == NULL) return -1;

    // Potem zmienić na to odpowiednie, odczytać ścieżke, a następnie wrócić 
    if (chdir(dir) != 0) return -1;
    realpath(target,path);
    if (chdir(pwd) != 0) return -1;
    return 0;
}

int is_descendant_of(const char *A, const char *B){
    // Wymagane absolutne ścieżki
    size_t lenA = strlen(A);
    size_t lenB = strlen(B);

    // B musi być krótsza lub równa, a obie ścieżki muszą zaczynać się od '/'
    if (lenB > lenA || A[0] != '/' || B[0] != '/') 
        return 0;

    // Jeśli B jest "/"
    if (strcmp(B, "/") == 0) 
        return 0;

    // Czy A zaczyna się od B?
    if (strncmp(A, B, lenB) != 0)
        return 0;

    // A zaczyna się od B — teraz sprawdzamy granicę katalogu
    // Przykład błędny: B="/home/us", A="/home/user" - prefix pasuje, ale to nie jest przodek
    if (A[lenB] == '\0') {
        // są identyczne
        return 1;
    }
    if (A[lenB] == '/') {

        // np. B="/home/user", A="/home/user/docs"

        return 1;
    }

    return 0;
}

int copy_file(const char *src, const char *dst, const copy_info* info)
{
    //Proste przepisanie uzywajace 8192 bufora
    //Możliwe że będzie trzeba go powiększać
    int in, out;
    char buf[8192];
    ssize_t n;

    in = open(src, O_RDONLY);
    if (in < 0) return -1;

    out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (out < 0) { close(in); return -1; }

    while ((n = read(in, buf, sizeof(buf))) > 0) {
        if (write(out, buf, n) != n) {
            close(in); close(out);
            return -1;
        }
    }

    close(in);
    close(out);
    return (n < 0) ? -1 : 0;
}

int copy_symlink(const char *src, const char *dst, const copy_info* info)
{
    char target[PATH_MAX];
    ssize_t len = readlink(src, target, sizeof(target) - 1);
    if (len < 0) return -1;
    target[len] = '\0';


    //Musimy tutaj sie pobawic w ścieżki bezwględne
    //Gdyż sytuacja wymaga dokładnego sprawdzenia gdzie wskazuje symlink

    char real_target_path[PATH_MAX];
    char real_root_path[PATH_MAX];
    char real_dest_root_path[PATH_MAX];
    get_real_symlink_path(src,real_target_path);
    realpath(info->root_src,real_root_path);
    realpath(info->root_dst,real_dest_root_path);
    if(is_descendant_of(real_target_path,real_root_path)){

        //Jest spokrewniony
        if(target[0] == '/'){
            //Jeśli jest bezwzględny
            char new_target[PATH_MAX];
            strcpy(new_target,real_dest_root_path);
            strcat(new_target,real_target_path+strlen(real_root_path));
            return symlink(new_target,dst);
        }
        else{
            //Jeśli nie
            return symlink(target,dst);
        }
    }
    else{
        //Nie jest spokrewniony
        return symlink(target, dst);
    }
}

// Tutaj tylko deklaracja, bo defincja będzie używała poźniejszych funkcji
int copy_directory(const char *src, const char *dst, const copy_info* info);

int copy_entry(const char *src_path, const char *dst_path, const copy_info* info)
{

    //Copy entry deleguje prace do innych funkcji w zależności od typu

    struct stat st;

    if (lstat(src_path, &st) < 0) {
        perror("lstat");
        return -1;
    }

    if (S_ISREG(st.st_mode)) {
        return copy_file(src_path, dst_path, info);

    } else if (S_ISLNK(st.st_mode)) {
        return copy_symlink(src_path, dst_path, info);

    } else if (S_ISDIR(st.st_mode)) {
        return copy_directory(src_path, dst_path, info);

    } else {
        fprintf(stderr, "Unsupported file type: %s\n", src_path);
        return -1;
    }
}

int copy_directory(const char *src, const char *dst, const copy_info* info)
{
    //Kopiowanie dyrektoriów  

    DIR *dir;
    struct dirent *entry;

    //Sprawdzamy czy możemy bezpiecznie otworzyć src
    
    if (mkdir(dst, 0777) < 0 && errno != EEXIST) {
        perror("mkdir");
        return -1;
    }

    dir = opendir(src);
    if (!dir) {
        perror("opendir");
        return -1;
    }

    //Idziemy kolejno po każdym pliku wewnątrz

    while ((entry = readdir(dir)) != NULL) {

        //W kopiowaniu pomijamy pliki "." oraz ".."

        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        char src_path[PATH_MAX];
        char dst_path[PATH_MAX];

        //Zdobywamy pełne adresy pliku src and dst

        snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);
        snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, entry->d_name);

        //Wykonujemy rekurencyjne kopiowanie

        if (copy_entry(src_path, dst_path, info) < 0) {
            closedir(dir);
            return -1;
        }
    }

    closedir(dir);
    return 0;
}
