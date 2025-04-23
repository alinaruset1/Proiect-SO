#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

pid_t pid_monitor = -1;
int finish = 0;
int closing = 0;

void case_sigchld(int semnal) {
    int status;
    waitpid(pid_monitor, &status, 0);
    finish = 1;
    closing = 0;
    printf("Monitorul s-a oprit cu statusul: %d\n", WEXITSTATUS(status));
}

void write_in_txt(const char *command) 
{
    FILE *fis=fopen("monitor_cmd.txt","w");
    if(!fis) 
    {
        perror("Eroare la deschiderea fisierului de memorie comuna");
        return;
    }
    fprintf(fis, "%s\n", command);
    fclose(fis);
}

int main(void) 
{
    struct sigaction actiune;
    actiune.sa_handler = case_sigchld;
    sigemptyset(&actiune.sa_mask);
    actiune.sa_flags=0;
    sigaction(SIGCHLD, &actiune, NULL);
    
    char user_command[128];
    while (1) 
    {   
        if(!fgets(user_command,sizeof(user_command),stdin))
            continue;
        user_command[strcspn(user_command,"\n")]=0;
        
        if (closing && !finish) {
            printf("Nu mai poti scrie comenzi in timp ce monitorul se inchide.\n");
            continue;
        }
        
        if(strcmp(user_command,"start_monitor")==0)
        {
            if(pid_monitor>0 && !finish)
            {
                printf("Monitorul ruleaza deja\n");
                continue;
            }
            pid_monitor=fork();
            if(pid_monitor==0)
            {
                execlp("./treasure_manager", "treasure_manager", "--monitor", (char *)NULL);
                perror("Eroare la pornirea monitorului");
                exit(1);
            } 
            else if(pid_monitor<0)
            {
                perror("Eroare la fork");
            } 
            else
            {
                printf("Monitor pornit cu PID-ul %d\n",pid_monitor);
                finish=0;
            }
        } 
        else if(strncmp(user_command,"list_hunts",10)==0)
        {
            if(pid_monitor<=0 || finish)
            {
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
        else if(strcmp(user_command,"stop_monitor")==0)
        {
            if(pid_monitor<=0 || finish)
            {
                printf("Monitorul nu ruleaza\n");
                continue;
            }
            kill(pid_monitor,SIGTERM);
            closing = 1;
            printf("Astept sa se opreasca monitorul...\n");
        } 
        else if(strcmp(user_command,"exit")==0)
        {
            if(pid_monitor>0 && !finish)
            {
                printf("Monitorul inca ruleaza. Foloseste mai intai stop_monitor\n");
                continue;
            }
            break;
        } 
        else {
    		printf("Comandă necunoscută: %s\n", user_command);
    		printf("Comenzi disponibile: start_monitor, list_hunts, list_treasures, view_treasure, stop_monitor, exit\n");
	}
    }
    return 0;
}  
