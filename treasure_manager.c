#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

#define USERNAME_LEN 32
#define CLUE_LEN 128
#define LOG_FILE "logged_hunt"

typedef struct {
    int treasure_id;
    char username[USERNAME_LEN];
    double latitude;
    double longitude;
    char clue[CLUE_LEN];
    int value;
} Treasure;

void create_symlink_for_log(const char *hunt_id) {
    char target[256];
    char linkname[256];

    snprintf(target, sizeof(target), "%s/%s", hunt_id, LOG_FILE);
    snprintf(linkname, sizeof(linkname), "logged_hunt-%s", hunt_id);

    if (access(linkname, F_OK) == -1) {
        if (symlink(target, linkname) == -1) {
            perror("Eroare la crearea symlink-ului");
        }
    }
}

void log_action(const char *hunt_id, const char *action) {
    char log_path[256];
    snprintf(log_path, sizeof(log_path), "%s/%s", hunt_id, LOG_FILE);
    
    int log_fd = open(log_path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (log_fd == -1) {
        perror("Eroare la deschiderea fisierului de log");
        return;
    }

    time_t now = time(NULL);
    char log_entry[256];
    snprintf(log_entry, sizeof(log_entry), "[%s] %s\n", ctime(&now), action);
    
    write(log_fd, log_entry, strlen(log_entry));
    close(log_fd);

    create_symlink_for_log(hunt_id);
}

void add_treasure(const char *hunt_id) {
    char dir_path[256], file_path[256];
    snprintf(dir_path, sizeof(dir_path), "%s", hunt_id);
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);

    mkdir(dir_path, 0755);
    int fd_read = open(file_path, O_RDONLY);
    Treasure t;
    int new_id;
    printf("ID Comoara: ");
    scanf("%d", &new_id);
    if (fd_read != -1) {
        while (read(fd_read, &t, sizeof(Treasure)) > 0) {
            if (t.treasure_id == new_id) {
                printf("Exista deja o comoara cu ID-ul %d\n", new_id);
                close(fd_read);
                char msg[100];
                snprintf(msg, sizeof(msg), "Incercare duplicat ID %d", new_id);
                log_action(hunt_id, msg);
                return;
            }
        }
        close(fd_read);
    }

    int fd = open(file_path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd == -1) {
        perror("Eroare la deschiderea fișierului");
        return;
    }

    t.treasure_id = new_id;
    printf("Utilizator: "); scanf("%s", t.username);
    printf("Latitudine: "); scanf("%lf", &t.latitude);
    printf("Longitudine: "); scanf("%lf", &t.longitude);
    printf("Indiciu: "); scanf(" %[^\n]", t.clue);
    printf("Valoare: "); scanf("%d", &t.value);

    write(fd, &t, sizeof(Treasure));
    close(fd);

    log_action(hunt_id, "Adaugat comoara");
}

void list_treasures(const char *hunt_id) {
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);

    struct stat st;
    if (stat(file_path, &st) == -1) {
        perror("Eroare la accesarea fisierului");
        return;
    }

    printf("Vanatoare: %s\nDimensiune fisier: %ld bytes\nUltima modificare: %s\n", 
           hunt_id, st.st_size, ctime(&st.st_mtime));

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("Eroare la deschiderea fisierului");
        return;
    }

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) > 0) {
        printf("ID: %d, User: %s, Locatie: (%.6f, %.6f), Indiciu: %s, Valoare: %d\n",
               t.treasure_id, t.username, t.latitude, t.longitude, t.clue, t.value);
    }
    close(fd);
    log_action(hunt_id, "Listat comori");
}

void view_treasure(const char *hunt_id, int id) {
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);
    
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("Eroare fisier");
        return;
    } 
    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) > 0) {
        if (t.treasure_id == id) {
            printf("ID: %d, User: %s, Locatie: (%.6f, %.6f), Indiciu: %s, Valoare: %d\n",
                   t.treasure_id, t.username, t.latitude, t.longitude, t.clue, t.value);
            char msg[100];
            snprintf(msg, sizeof(msg), "Vizualizat comoara ID %d", id);
            log_action(hunt_id, msg);
            close(fd);
            return;
        }
    }
    printf("Comoara cu ID-ul %d nu a fost gasita\n", id);
    close(fd);
}

void remove_treasure(const char *hunt_id, int id) {
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);
    
    FILE *fin = fopen(file_path, "rb");
    if (!fin) {
        perror("Eroare la deschidere pentru citire");
        return;
    }
    
    FILE *fout = fopen("temp.dat", "wb");
    if (!fout) {
        perror("Eroare la creare fisier temporar");
        fclose(fin);
        return;
    }
    
    Treasure t;
    int found = 0;
    while (fread(&t, sizeof(Treasure), 1, fin)) {
        if (t.treasure_id == id) {
            found = 1;
        } else {
            fwrite(&t, sizeof(Treasure), 1, fout);
        }
    }
    
    fclose(fin);
    fclose(fout);
    if (found) {
        remove(file_path);
        rename("temp.dat", file_path);
        char msg[100];
        snprintf(msg, sizeof(msg), "Sters comoara ID %d", id);
        log_action(hunt_id, msg);
        printf("Comoara cu ID %d a fost stearsa\n", id);
    } else {
        printf("Comoara cu ID-ul %d nu a fost gasita\n", id);
        remove("temp.dat");
    }
}

void remove_hunt(const char *hunt_id) {
  
    log_action(hunt_id, "Sters vanatoare");
    char command[256];
    snprintf(command, sizeof(command), "rm -rf %s", hunt_id);
    system(command);

    char symlink_name[256];
    snprintf(symlink_name, sizeof(symlink_name), "logged_hunt-%s", hunt_id);
    unlink(symlink_name);  

    
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Utilizare: treasure_manager --comanda <hunt_id> [optiuni]\n");
        return 1;
    }

    if (strcmp(argv[1], "--add") == 0) {
        add_treasure(argv[2]);
    } else if (strcmp(argv[1], "--list") == 0) {
        list_treasures(argv[2]);
    } else if (strcmp(argv[1], "--remove_hunt") == 0) {
        remove_hunt(argv[2]);
    } else if (strcmp(argv[1], "--view") == 0 && argc == 4) {
        view_treasure(argv[2], atoi(argv[3]));
    } else if (strcmp(argv[1], "--remove_treasure") == 0 && argc == 4) {
        remove_treasure(argv[2], atoi(argv[3]));
    } else {
        printf("Comanda invalida.\n");
    }
    return 0;
}

