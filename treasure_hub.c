#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

pid_t pid_monitor = -1;
int monitor_terminat = 0;
int monitor_inchid = 0;

void tratare_sigchld(int semnal) {
    int status;
    waitpid(pid_monitor, &status, 0);
    monitor_terminat = 1;
    monitor_inchid = 0;
    printf("Monitorul s-a oprit cu statusul: %d\n", WEXITSTATUS(status));
}

void scrie_comanda(const char *comanda) 
{
    FILE *fis=fopen("monitor_cmd.txt","w");
    if(!fis) 
    {
        perror("Eroare la deschiderea fisierului de comanda");
        return;
    }
    fprintf(fis, "%s\n", comanda);
    fclose(fis);
}

int main(void) 
{
    struct sigaction actiune;
    actiune.sa_handler = tratare_sigchld;
    sigemptyset(&actiune.sa_mask);
    actiune.sa_flags=0;
    sigaction(SIGCHLD, &actiune, NULL);
    char comanda_utilizator[128];
    while (1) 
    {   
        if(!fgets(comanda_utilizator,sizeof(comanda_utilizator),stdin))
            continue;
        comanda_utilizator[strcspn(comanda_utilizator,"\n")]=0;
        
        if (monitor_inchid && !monitor_terminat) {
            printf("Monitorul se închide. Așteaptă finalizarea...\n");
            continue;
        }
        
        if(strcmp(comanda_utilizator,"start_monitor")==0)
        {
            if(pid_monitor>0 && !monitor_terminat)
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
                monitor_terminat=0;
            }
        } 
        else if(strncmp(comanda_utilizator,"list_hunts",10)==0)
        {
            if(pid_monitor<=0 || monitor_terminat)
            {
                printf("Monitorul nu ruleaza\n");
                continue;
            }
            scrie_comanda("list_hunts");
            kill(pid_monitor, SIGUSR1);
        } 
        else if(strncmp(comanda_utilizator,"list_treasures",14)==0)
        {
            if(pid_monitor<= 0 || monitor_terminat)
            {
                printf("Monitorul nu ruleaza\n");
                continue;
            }
            char vanatoare[64], id[64];
            if (sscanf(comanda_utilizator, "list_treasures %s", vanatoare) != 1){
        		printf("Format corect: list_treasures <hunt_id> <treasure_id>\n");
        		continue;
    		}
            char comanda_finala[128];
            sprintf(comanda_finala, "list_treasures %s",vanatoare);
            scrie_comanda(comanda_finala);
            kill(pid_monitor, SIGUSR1);
        } 
        else if(strncmp(comanda_utilizator,"view_treasure",13)==0){
    		if(pid_monitor<=0 || monitor_terminat)
    		{
        		printf("Monitorul nu ruleaza\n");
        		continue;
    		}		

    		char comoara[64], id[64];
    		if (sscanf(comanda_utilizator, "view_treasure %s %s", comoara, id) != 2){
        		printf("Format corect: view_treasure <hunt_id> <treasure_id>\n");
        		continue;
    		}

    		char comanda_finala[128];
   		sprintf(comanda_finala,"view_treasure %s %s", comoara, id);
    		scrie_comanda(comanda_finala);
    		kill(pid_monitor, SIGUSR1);
}
        else if(strcmp(comanda_utilizator,"stop_monitor")==0)
        {
            if(pid_monitor<=0 || monitor_terminat)
            {
                printf("Monitorul nu ruleaza\n");
                continue;
            }
            kill(pid_monitor,SIGTERM);
            monitor_inchid = 1;
            printf("Astept sa se opreasca monitorul...\n");
        } 
        else if(strcmp(comanda_utilizator,"exit")==0)
        {
            if(pid_monitor>0 && !monitor_terminat)
            {
                printf("Monitorul inca ruleaza. Foloseste mai intai stop_monitor\n");
                continue;
            }
            break;
        } 
        else {
    		printf("Comandă necunoscută: %s\n", comanda_utilizator);
    		printf("Comenzi disponibile: start_monitor, list_hunts, list_treasures, view_treasure, stop_monitor, exit\n");
	}
    }
    return 0;
}  
