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

void log_action(const char *hunt_id, const char *action) {
    char log_path[256];
    snprintf(log_path, sizeof(log_path), "%s/%s", hunt_id, LOG_FILE);
    
    int log_fd = open(log_path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (!log_fd) {
        perror("Eroare la deschiderea fisierului de log");
        return;
    }

    time_t now = time(NULL);
    char log_entry[256];
    snprintf(log_entry, sizeof(log_entry), "%s%s\n", ctime(&now), action);
    
    write(log_fd, log_entry, strlen(log_entry));
    close(log_fd);
}


void add_treasure(const char *hunt_id) {
    char dir_path[256], file_path[256];
    snprintf(dir_path, sizeof(dir_path), "%s", hunt_id);
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);

    mkdir(dir_path, 0755);

    int fd = open(file_path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd == -1) {
        perror("Eroare la deschiderea fisierului");
        return;
    }

    Treasure t;
    printf("ID Comoară: "); scanf("%d", &t.treasure_id);
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
}

void remove_hunt(const char *hunt_id) {
    char command[256];
    snprintf(command, sizeof(command), "rm -rf %s", hunt_id);
    system(command);
    log_action(hunt_id, "Sters vanatoare");
}


int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Eroare la argumente\n");
        return 1;
    }

    if (strcmp(argv[1], "--add") == 0) {
        add_treasure(argv[2]);
    } else if (strcmp(argv[1], "--list") == 0) {
        list_treasures(argv[2]);
    } else if (strcmp(argv[1], "--remove_hunt") == 0) {
        remove_hunt(argv[2]);
    }
    else {
        printf("Comanda invalida.\n");
    }
    return 0;
}
