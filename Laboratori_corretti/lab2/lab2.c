#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>

void print_file_details(const char *path) {
    struct stat file_stat;
    struct passwd *pwd;
    struct group *grp;

    // Ottenimento informazioni sul file
    if (stat(path, &file_stat) != 0) {
        perror("Errore nel recuperare le informazioni sul file");
        return;
    }

    // Ottenimento proprietario del file
    pwd = getpwuid(file_stat.st_uid);
    if (pwd == NULL) {
        perror("Errore nel recuperare il nome del proprietario");
        return;
    }

    // Ottenimento gruppo del file
    grp = getgrgid(file_stat.st_gid);
    if (grp == NULL) {
        perror("Errore nel recuperare il nome del gruppo");
        return;
    }

    // Stampa i dettagli del file nel formato richiesto
    printf("Node: %s\n", path);
    printf("Inode: %lo\n", file_stat.st_ino);
    printf("Type: ");
    if (S_ISDIR(file_stat.st_mode)) {
        printf("directory\n");
    } else if (S_ISREG(file_stat.st_mode)) {
        printf("file\n");
    } else if (S_ISLNK(file_stat.st_mode)){
        printf("symbolic link\n");
    } else if (S_ISFIFO(file_stat.st_mode)){
        printf("FIFO\n");
    } else {
        printf("other\n");
    }
    printf("Size: %ld\n", file_stat.st_size);
    printf("Owner: %d %s\n", file_stat.st_uid, pwd->pw_name);
    printf("Group: %d %s\n", file_stat.st_gid, grp->gr_name);
    printf("\n");
}

void explore_directory(const char *dir_path) {
    DIR *dir;
    struct dirent *entry;

    // Apri la directory
    dir = opendir(dir_path);
    if (dir == NULL) {
        perror("Errore nell'apertura della directory");
        return;
    }

    // Leggi tutti gli elementi della directory
    while ((entry = readdir(dir)) != NULL) {
        // Ignora le directory "." e ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Costruisci il percorso completo del file/directory
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        // Stampare i dettagli del file/directory
        print_file_details(full_path);

        // Se è una directory, esplorala ricorsivamente
        if (entry->d_type == DT_DIR) {
            explore_directory(full_path);
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Utilizzo: %s <percorso_cartella>\n", argv[0]);
        return 1;
    }

    char *root_dir = argv[1];

    print_file_details(root_dir);
    explore_directory(root_dir);

    return 0;
}