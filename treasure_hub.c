#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>

pid_t pid_monitor = -1;
int finish = 0;
int closing = 0;

// Signal handler for SIGCHLD (monitor process termination)
void case_sigchld(int semnal) {
    int status;
    pid_t ret = waitpid(pid_monitor, &status, WNOHANG);
    if (ret == pid_monitor) {
        finish = 1;
        closing = 0;
        printf("Monitorul s-a oprit cu statusul: %d\n", WEXITSTATUS(status));
    }
}

// Function to write a command to the "monitor_cmd.txt" file
void write_in_txt(const char *command) {
    FILE *fis = fopen("monitor_cmd.txt", "w");
    if (!fis) {
        perror("Eroare la deschiderea fisierului de comenzi");
        return;
    }
    fprintf(fis, "%s\n", command);  // Write the command
    fflush(fis);
    fclose(fis);
}

// Function to read and print data from a pipe
void citire_din_pipe(int pipefd) {
    char buffer[1024];
    ssize_t n;
    while ((n = read(pipefd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[n] = '\0';  // Null-terminate buffer
        printf("%s", buffer);
        fflush(stdout);  // Ensure immediate output
    }
    if (n == -1) {
        perror("Eroare la citirea din pipe");
    }
}

// Main function
int main(void) {
    struct sigaction actiune;
    actiune.sa_handler = case_sigchld;
    sigemptyset(&actiune.sa_mask);
    actiune.sa_flags = SA_RESTART | SA_NOCLDSTOP;  // Restart system call if interrupted by a signal
    sigaction(SIGCHLD, &actiune, NULL);

    char user_command[128];
    while (1) {
        if (!fgets(user_command, sizeof(user_command), stdin))
            continue;
        user_command[strcspn(user_command, "\n")] = 0;  // Remove newline

        if (closing && !finish) {
            printf("Nu mai poti scrie comenzi in timp ce monitorul se inchide.\n");
            continue;
        }

        if (strcmp(user_command, "start_monitor") == 0) {
            if (pid_monitor > 0 && !finish) {
                printf("Monitorul ruleaza deja\n");
                continue;
            }
            pid_monitor = fork();
            if (pid_monitor == 0) {
                execlp("./treasure_manager", "treasure_manager", NULL);
                perror("Eroare la pornirea monitorului");
                exit(1);
            } else if (pid_monitor < 0) {
                perror("Eroare la fork");
            } else {
                printf("Monitor pornit cu PID-ul %d\n", pid_monitor);
                finish = 0;
            }
        } else if (strncmp(user_command, "list_hunts", 10) == 0) {
            if (pid_monitor <= 0 || finish) {
                printf("Monitorul nu ruleaza\n");
                continue;
            }
            write_in_txt("list_hunts");
            kill(pid_monitor, SIGUSR1);
        }
          else if(strncmp(user_command,"list_treasures",14)==0)
        {
            if(pid_monitor<= 0 || finish)
            {
                printf("Monitorul nu ruleaza\n");
                continue;
            }
            char vanatoare[64];
            if (sscanf(user_command, "list_treasures %s", vanatoare) != 1){
        		printf("Format corect: list_treasures hunt_id treasure_id\n");
        		continue;
    		}
            char comanda_finala[256];
            sprintf(comanda_finala, "list_treasures %s",vanatoare);
            write_in_txt(comanda_finala);
            kill(pid_monitor, SIGUSR1);
        } 
        else if(strncmp(user_command,"view_treasure",13)==0){
    		if(pid_monitor<=0 || finish)
    		{
        		printf("Monitorul nu ruleaza\n");
        		continue;
    		}		

    		char comoara[64], id[64];
    		if (sscanf(user_command, "view_treasure %s %s", comoara, id) != 2){
        		printf("Format corect: view_treasure <hunt_id> <treasure_id>\n");
        		continue;
    		}

    		char comanda_finala[256];
   		sprintf(comanda_finala,"view_treasure %s %s", comoara, id);
    		write_in_txt(comanda_finala);
    		kill(pid_monitor, SIGUSR1);
}
        else if (strncmp(user_command, "calculate_score", 15) == 0) {
        if(pid_monitor<=0 || finish)
    		{
        		printf("Monitorul nu ruleaza\n");
        		continue;
    		}	
            char hunt_id[64];
            if (sscanf(user_command, "calculate_score %s", hunt_id) != 1) {
                printf("Correct usage: calculate_score <hunt_id>\n");
                continue;
            }

            

            int pipefd[2];
            if (pipe(pipefd) == -1) {
                perror("[DEBUG] Failed to create pipe");
                continue;
            }

            pid_t pid = fork();
            if (pid == -1) {
                perror("[DEBUG] Error during fork");
                close(pipefd[0]);
                close(pipefd[1]);
                continue;
            }

            if (pid == 0) {
                // Child process
                close(pipefd[0]); // Close unused read end
                dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout and stderr
                dup2(pipefd[1], STDERR_FILENO);
                close(pipefd[1]); // Close write end

                execlp("./calculate_score", "./calculate_score", hunt_id, NULL);
               
                exit(EXIT_FAILURE);
            } else {
                // Parent process
                close(pipefd[1]); // Close unused write end

               

                char buffer[1024];
                ssize_t bytes_read;
                while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
                    buffer[bytes_read] = '\0';
                    printf("%s", buffer);
                }

                if (bytes_read == -1) {
                    perror("[DEBUG] Error occurred while reading from pipe.");
                } 
                close(pipefd[0]);


               
                int status;
                if (waitpid(pid, &status, 0) == -1) {
                    perror("[DEBUG] waitpid failed");
                    return 1;
                }
                }


        } else if (strcmp(user_command, "stop_monitor") == 0) {
            if (pid_monitor <= 0 || finish) {
                printf("Monitorul nu ruleaza\n");
                continue;
            }
            kill(pid_monitor, SIGTERM);
            closing = 1;
            printf("Astept sa se opreasca monitorul...\n");
        } else if (strcmp(user_command, "exit") == 0) {
            if (pid_monitor > 0 && !finish) {
                printf("Monitorul inca ruleaza. Foloseste mai intai stop_monitor\n");
                continue;
            }
            break;
        } else {
            printf("Comandă necunoscută: %s\n", user_command);
            printf("Comenzi disponibile: start_monitor, list_hunts, view_treasure, list_treasures, calculate_score, stop_monitor, exit\n");
        }
    }
    return 0;
}
