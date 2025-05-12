#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

typedef struct {
    char treasure_id;
    char username[32];
    double latitude;
    double longitude;
    char clue[128];
    int value;
} Treasure;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Utilizare: %s <hunt_id>\n", argv[0]);
        return 1;
    }

    char initial_dir[256];
    if (getcwd(initial_dir, sizeof(initial_dir)) == NULL) {
        perror("Eroare la obținerea directorului curent înainte de schimbare");
        return 1;
    }

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "./%s/treasures.dat", argv[1]);
    FILE *file = fopen(filepath, "rb");

    if (!file) {
        perror("Eroare la deschiderea fișierului treasures.dat");
        return 1;
    }

    Treasure t;
    struct UserScore {
        char username[32];
        int score;
        struct UserScore *next;
    } *head = NULL;

    while (fread(&t, sizeof(Treasure), 1, file) == 1) {
        struct UserScore *current = head;
        while (current) {
            if (strcmp(current->username, t.username) == 0) {
                current->score += t.value;
                break;
            }
            current = current->next;
        }
        if (!current) {
            struct UserScore *new_user = malloc(sizeof(struct UserScore));
            if (new_user == NULL) {
                fprintf(stderr, "Eroare la alocarea memoriei pentru structura UserScore.\n");
                fclose(file);
                while (head) { // Free any previously allocated memory
                    struct UserScore *temp = head;
                    head = head->next;
                    free(temp);
                }
                return 1;
            }

            // Safely copy the username
            strncpy(new_user->username, t.username, sizeof(new_user->username) - 1);
            new_user->username[sizeof(new_user->username) - 1] = '\0';
            new_user->score = t.value;
            new_user->next = head;
            head = new_user;
        }
    }

    fclose(file);

    struct UserScore *current = head;
    while (current) {
        printf("Utilizator: %s | Scor: %d\n", current->username, current->score);
        struct UserScore *temp = current;
        current = current->next;
        free(temp);
    }

    if (chdir("..") != 0) {
        perror("Eroare la schimbarea directorului");
        return 1;
    }

    // Restore the original directory before exiting
    if (chdir(initial_dir) != 0) {
        perror("Eroare la restaurarea directorului inițial");
        return 1;
    }

    
    fflush(stdout);
    return 0;
}
