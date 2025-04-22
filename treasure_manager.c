#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <dirent.h>

#define CMD_FILE "monitor_command.txt"
#define TREASURE_FILE "treasures.dat"
#define LOG_FILE "logged_hunt"


typedef struct {
    char treasure_id;
    char username[32];
    double latitude;
    double longitude;
    char clue[128];
    int value;
} Treasure;


void create_symlink_for_log(const char *hunt_id) {
    char target[256];
    char linkname[256];

    snprintf(target, sizeof(target), "%s/%s", hunt_id, LOG_FILE);
    snprintf(linkname, sizeof(linkname), "logged_hunt-%s", hunt_id);

    if (access(linkname, F_OK) == -1) {
        if (symlink(target, linkname) == -1) {
            perror("Can't create simlink");
        }
    }
}

void log_action(const char *hunt_id, const char *action) {
    char log_path[256];
    snprintf(log_path, sizeof(log_path), "%s/%s", hunt_id, LOG_FILE);
    
    int log_fd = open(log_path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (log_fd == -1) {
        perror("Can't open log file");
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
    printf("Treasure ID: ");
    scanf("%d", &new_id);
    if (fd_read != -1) {
        while (read(fd_read, &t, sizeof(Treasure)) > 0) {
            if (t.treasure_id == new_id) {
                printf("There is already a treasure with ID %d\n", new_id);
                close(fd_read);
                char msg[100];
                snprintf(msg, sizeof(msg), "Attempt to add a duplicate with ID %d", new_id);
                log_action(hunt_id, msg);
                return;
            }
        }
        close(fd_read);
    }

    int fd = open(file_path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd == -1) {
        perror("Can't open the file");
        return;
    }

    t.treasure_id = new_id;
    printf("User: "); scanf("%s", t.username);
    printf("Latitude: "); scanf("%lf", &t.latitude);
    printf("Longitude: "); scanf("%lf", &t.longitude);
    printf("Clue: "); scanf(" %[^\n]", t.clue);
    printf("Value: "); scanf("%d", &t.value);

    write(fd, &t, sizeof(Treasure));
    close(fd);

    log_action(hunt_id, "Treasure added");
}

void list_treasures(const char *hunt_id) {
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);

    struct stat st;
    if (stat(file_path, &st) == -1) {
        perror("Can't open the file");
        return;
    }

    printf("Hunt: %s\nFile size: %ld bytes\nLast modified: %s\n", 
           hunt_id, st.st_size, ctime(&st.st_mtime));

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("Can't open the file");
        return;
    }

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) > 0) {
        printf("ID: %d, User: %s, GPS Coordinates: (%.6f, %.6f), Clue: %s, Value: %d\n",
               t.treasure_id, t.username, t.latitude, t.longitude, t.clue, t.value);
    }
    close(fd);
    log_action(hunt_id, "Treasures listed");
}

void view_treasure(const char *hunt_id, int id) {
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);
    
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("File error");
        return;
    } 
    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) > 0) {
        if (t.treasure_id == id) {
            printf("ID: %d, User: %s, GPS Coordinates: (%.6f, %.6f), Clue: %s, Value: %d\n",
                   t.treasure_id, t.username, t.latitude, t.longitude, t.clue, t.value);
            char msg[100];
            snprintf(msg, sizeof(msg), "Viewed treasure ID %d", id);
            log_action(hunt_id, msg);
            close(fd);
            return;
        }
    }
    printf("Treasure with ID %d was not found\n", id);
    close(fd);
}

void remove_treasure(const char *hunt_id, int id) {
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);
    
    FILE *fin = fopen(file_path, "rb");
    if (!fin) {
        perror("Unable to open file in read mode");
        return;
    }
    
    FILE *fout = fopen("temp.dat", "wb");
    if (!fout) {
        perror("Unable to create temporary file");
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
        snprintf(msg, sizeof(msg), "Deleted treasure with ID %d", id);
        log_action(hunt_id, msg);
        printf("Successfully deleted treasure with ID %d\n", id);
    } else {
        printf("No treasure found with ID %d\n", id);
        remove("temp.dat");
    }
}
/*
void remove_hunt(const char *hunt_id) {
  
    log_action(hunt_id, "Deleted hunt");
    char command[256];
    snprintf(command, sizeof(command), "rm -rf %s", hunt_id);
    system(command);

    char symlink_name[256];
    snprintf(symlink_name, sizeof(symlink_name), "logged_hunt-%s", hunt_id);
    unlink(symlink_name);  

    
}
*/


void remove_hunt(const char *hunt_id) {
	char treasure_file[256], log_file[256], symlink_name[256];
	snprintf(treasure_file, sizeof(treasure_file), "%s/treasures.dat", hunt_id);
	snprintf(log_file, sizeof(log_file), "%s/%s", hunt_id, LOG_FILE);
	snprintf(symlink_name, sizeof(symlink_name), "logged_hunt-%s", hunt_id);

	
	if (access(treasure_file, F_OK) == 0) {
		FILE *f = fopen(treasure_file, "wb");
		if (f)
			 fclose(f); 
		if (remove(treasure_file) != 0) {
			perror("Unable to delete treasures.dat file");
		}
	}

	
	if (access(log_file, F_OK) == 0) {
		FILE *f = fopen(log_file, "wb");
		if (f) 
			fclose(f); 
		if (remove(log_file) != 0) {
			perror("Unable to delete logged_hunt file");
		}
	}

	unlink(symlink_name);
	if (rmdir(hunt_id) != 0) {
		perror("Unable to delete hunt directory");
	} 
	else {
		printf("Hunt '%s' was completely deleted.\n", hunt_id);
	}
}

void listeaza_vanatori() 
{
    DIR *dir = opendir(".");
    struct dirent *entry;
    char cale[256];
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
        {
            
            size_t len = strlen(entry->d_name) + strlen(TREASURE_FILE) + 2; // +2 pentru "/" si '\0'

            if (len >= sizeof(cale)) 
            {
                // Dacă dimensiunea depaseste buffer-ul, ignor acest director
                continue;
            }

            
            snprintf(cale, sizeof(cale), "%s/%s", entry->d_name, TREASURE_FILE);
            FILE *fis = fopen(cale, "rb");
            if (!fis)
                continue;
            int count = 0;
            Treasure c;
            while (fread(&c, sizeof(Treasure), 1, fis) == 1)
                count++;
            fclose(fis);
            printf("Vanatoare: %s | Comori: %d\n", entry->d_name, count);
        }
    }
    closedir(dir);
}


void gestioneaza_comanda(int semnal) 
{
    FILE *fis=fopen("monitor_cmd.txt","r");
    if(!fis) 
    {
        perror("Eroare la deschiderea fisierului de comenzi");
        return;
    }
    char comanda[64],vanatoare[64];
    int id;
    fscanf(fis,"%s",comanda);
    if(strcmp(comanda,"list_hunts")==0)
    {
        listeaza_vanatori();
    } 
    else if(strcmp(comanda,"list_treasures")==0)
    {       
        fscanf(fis,"%s",vanatoare);
        vanatoare[strcspn(vanatoare,"\n")]='\0';
        list_treasures(vanatoare);
        strcpy(vanatoare,"");
    } 

    else if(strcmp(comanda,"view_treasure")==0)

    {   
        fscanf(fis,"%s",vanatoare);
        fscanf(fis,"%d",&id);
        view_treasure(vanatoare,id);
        strcpy(vanatoare,"");
    }

    fclose(fis);

}

void opreste_monitor(int semnal) 
{
    printf("Monitorul se opreste...\n");
    usleep(3000000);
    exit(0);

}



int main(int argc,char *argv[])
{
    if(argc==2 && strcmp(argv[1],"--monitor")==0)
    {
        struct sigaction sa;
        sa.sa_handler = gestioneaza_comanda;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGUSR1, &sa, NULL);
        struct sigaction sa_term;
        sa_term.sa_handler = opreste_monitor;
        sigemptyset(&sa_term.sa_mask);
        sa_term.sa_flags = 0;
        sigaction(SIGTERM, &sa_term, NULL);

        while(1) 
        {
            pause();
        }

    }

    if (strcmp(argv[1], "--add") == 0) {
        add_treasure(argv[2]);
    } else if (strcmp(argv[1], "--list") == 0 && argc==3) {
        list_treasures(argv[2]);
    } else if (strcmp(argv[1], "--remove_hunt") == 0) {
        remove_hunt(argv[2]);
    } else if (strcmp(argv[1], "--view") == 0 && argc == 4) {
        view_treasure(argv[2], atoi(argv[3]));
    } else if (strcmp(argv[1], "--remove_treasure") == 0 && argc == 4) {
        remove_treasure(argv[2], atoi(argv[3]));
    } else {
        printf("Invalid command.\n");
    }
    return 0;
}

