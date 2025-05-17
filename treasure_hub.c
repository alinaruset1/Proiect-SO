#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#define BUFFER_SIZE 4096
#define DELIMITER '\x1E'
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


void read_until_delimiter(int fd, char *buffer, size_t max_size) {
    size_t i = 0;
    char c;
    while (i < max_size - 1 && read(fd, &c, 1) == 1) {
        if (c == DELIMITER) break;
        buffer[i++] = c;
    }
    buffer[i] = '\0';
}

void case_sigchld(int semnal) {
    int status;
    pid_t ret = waitpid(pid_monitor, &status, WNOHANG);
    if (ret == pid_monitor) {
        finish = 1;
        closing = 0;
        printf("Monitor stopped with status: %d\n", WEXITSTATUS(status));
    }
}


void write_in_txt(const char *command) {
    FILE *fis = fopen("monitor_cmd.txt", "w");
    if (!fis) {
        perror("Can not open the command file");
        return;
    }
    fprintf(fis, "%s\n", command);  
    fflush(fis);
    fclose(fis);
}


void citire_din_pipe(int pipefd) {
    char buffer[1024];
    ssize_t n;
    while ((n = read(pipefd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[n] = '\0';  
        printf("%s", buffer);
        fflush(stdout); 
    }
    if (n == -1) {
        perror("Error reading from pipe");
    }
}


int main(void) {
    struct sigaction actiune;
    actiune.sa_handler = case_sigchld;
    sigemptyset(&actiune.sa_mask);
    actiune.sa_flags = SA_RESTART | SA_NOCLDSTOP;  
    sigaction(SIGCHLD, &actiune, NULL);

    char user_command[128];
    while (1) {
        if (!fgets(user_command, sizeof(user_command), stdin))
            continue;
        user_command[strcspn(user_command, "\n")] = 0;  

        if (closing && !finish) {
            printf("You can no longer enter commands while the monitor is shutting down.\n");
            continue;
        }

        if (strcmp(user_command, "start_monitor") == 0) {
            if (pid_monitor > 0 && !finish) {
                printf("The monitor is already running.\n");
                continue;
            }
            pid_monitor = fork();
            if (pid_monitor == 0) {
                execlp("./treasure_manager", "treasure_manager", NULL);
                perror("Error starting the monitor.");
                exit(1);
            } else if (pid_monitor < 0) {
                perror("Fork error.");
            } else {
                printf("Monitor started with PID %d\n", pid_monitor);
                finish = 0;
            }
        } else if (strncmp(user_command, "list_hunts", 10) == 0) {
            if (pid_monitor <= 0 || finish) {
                printf("The monitor is not running.\n");
                continue;
            }
            write_in_txt("list_hunts");
            kill(pid_monitor, SIGUSR1);
        }
          else if(strncmp(user_command,"list_treasures",14)==0)
        {
            if(pid_monitor<= 0 || finish)
            {
                printf("The monitor is not running.\n");
                continue;
            }
            char vanatoare[64];
            if (sscanf(user_command, "list_treasures %s", vanatoare) != 1){
        		printf("Correct format: list_treasures hunt_id treasure_id\n");
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
        		printf("The monitor is not running.\n");
        		continue;
    		}		

    		char comoara[64], id[64];
    		if (sscanf(user_command, "view_treasure %s %s", comoara, id) != 2){
        		printf("Correct format: view_treasure <hunt_id> <treasure_id>\n");
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
        		printf("The monitor is not running.\n");
        		continue;
    		}	
            char hunt_id[64];
            if (sscanf(user_command, "calculate_score %s", hunt_id) != 1) {
                printf("Correct format: calculate_score <hunt_id>\n");
                continue;
            }

            int pipefd[2];
            if (pipe(pipefd) == -1) {
                perror("Failed to create pipe");
                continue;
            }

            pid_t pid = fork();
            if (pid == -1) {
                perror("Error during fork");
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
                char buffer[BUFFER_SIZE];
    		read_until_delimiter(pipefd[0], buffer, BUFFER_SIZE);
    		printf("%s", buffer);
                close(pipefd[0]);               
                int status;
                if (waitpid(pid, &status, 0) == -1) {
                    perror("waitpid failed");
                    return 1;
                }
           }


        } else if (strcmp(user_command, "stop_monitor") == 0) {
            if (pid_monitor <= 0 || finish) {
                printf("The monitor is not running.\n");
                continue;
            }
            kill(pid_monitor, SIGTERM);
            closing = 1;
            printf("Waiting for the monitor to shut down...\n");
        } else if (strcmp(user_command, "exit") == 0) {
            if (pid_monitor > 0 && !finish) {
                printf("The monitor is still running... Use stop_monitor first\n");
                continue;
            }
            break;
        } else {
            printf("Unknown command: %s\n", user_command);
            printf("Available: start_monitor, list_hunts, view_treasure, list_treasures, calculate_score, stop_monitor, exit\n");
        }
    }
    return 0;
}

