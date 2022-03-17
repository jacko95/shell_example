#include "smallsh.h"
#include <sys/types.h>


char *prompt = "Dare un comando>";

/* tratta una riga di input */
int procline(void){
    char *arg[MAXARG+1];	/* array di puntatori per runcommand */
    int toktype;	/* tipo del simbolo nel comando */
    int narg;	/* numero di argomenti considerati finora */
    int type;	/* FOREGROUND o BACKGROUND */
    narg = 0;
    bandiera = 0; //variabile flag

    while (1) { /* ciclo da cui si esce con il return */
    /* esegue un'azione a seconda del tipo di simbolo */
    /* mette un simbolo in arg[narg] */
        switch (toktype = gettok(&arg[narg])) { // mette ls &arg[narg], toktype è il tipo di segno(istruzione), riempie il vettore in posizione arg[narg] per ogni cella del vetore arg
	
            /* se argomento: passa al prossimo simbolo */
            //Contiene gli argomenti della riga di comando (tranne i caratteri speciali)
            case ARG://ls, nome file in cui si salva il risultato del comando
                if(bandiera == 1){
                    strcpy(f, arg[narg]);
                    narg--;
                }
                
                if (narg < MAXARG) //Se non si è ancora terminata la lettura della riga di comando
                    narg++; //Considera l'argomento(stringa) successivo
                
                break;

            /* se fine riga o ';' o '&' esegue il comando ora contenuto in arg, mettendo NULL per segnalare la fine degli argomenti: serve a execvp */
            case MAGGIORE://>
                bandiera = 1;
                break;
                
            case EOL:
            case SEMICOLON:
            case AMPERSAND://&
                type = (toktype == AMPERSAND) ? BACKGROUND : FOREGROUND; //if(toktype == AMPERSAND) type = BACKGROUND; else type = FOREGROUND;
                
                if (narg != 0) {
                    arg[narg] = NULL;
                    runcommand(arg, type, fd);//fa partire il processo figlio(comando)
                }
      
                /* se fine riga, procline e' finita */
                if (toktype == EOL) return 1;
      
                /* altrimenti (caso del comando terminato da ';' o '&') bisogna ricominciare a riempire arg dall'indice 0 */
                narg = 0;
                break;
        }
    }
}

//Si fa una if sul valore di where e se vale background non si fa la wait
/* esegue un comando , prende il vettore di stringhe già parsificato(suddiviso) e where che contiene il valore di foreground o background*/
void runcommand(char **cline, int where, int fd){
    pid_t pid;
    int exitstat, ret;
    
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    struct sigaction sa_ign;
    sa_ign.sa_handler = SIG_IGN;
    sigemptyset(&sa_ign.sa_mask);
    sa_ign.sa_flags = 0;
    
    
    //La chiamata di waitpid con parametro WNOHANG controlla tutti i processi FIGLIO (con -1 come primo argomento ne aspetta uno qualunque, se si passa il pid di un processo le dico di aspettare specificatamente quel processo) modificando il secondo parametro (cioè exitstat) e senza causare l'attesa del processo padre (colui che l'ha chiamata)
    //L'obiettivo è controllare periodicamente lo stato di terminazione dei processi figlio in background
    //CON WNOHANG LA CHIAMATA DI WAITPID NON È SOSPENSIVA
    pid_t c = waitpid(-1, &exitstat, WNOHANG); //In caso di successo (se restituisce piu di 0) ritorna il pid del processo terminato, se restituisce -1 non ci sono processi figlio da aspettare.
    if(c > 0){
        //La exitstat contiene lo stato di terminazione del processo
        //La WIFEXITED controlla se e come il processo è terminato 
        if(WIFEXITED(exitstat) == 1)//se il processo è terminato normalmente
            printf("processo con pid %d terminato correttamente\n", (int)c);
        else 
            printf("processo terminato non correttamente\n");
    }
    
    pid = fork(); //Si crea il nuovo processo che eseguirà il comando
    
    if (pid == (pid_t) -1) {
        perror("smallsh: fork fallita");
        return;
    }
    
    if(where == FOREGROUND){ //Se shell attende che prima venga eseguito il comando
           
        if (pid == (pid_t) 0){ //Se siamo nel processo figlio (cioè il  comando)
            sigaction(SIGINT, &sa, NULL);
                
            if(bandiera == 1){
                fd = creat(f, S_IRWXU); //fd = open(f, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU); 
                if(fd == -1) //Se la creat restituisce un errore
                    exit(1);
                
                close(1); //chiudiamo lo standard output
                dup2(fd, 1);
                close(fd);
            }
            
            execvp(*cline, cline);//execvp(percorso dell'eseguibile da sostituire primo termine, ambiente del processo da eseguire, cioè il percorso dell' eseguibile). Vettore cline contiene i comandi e gli argomenti(come 2 argomento che ha come cella 0 del vettore è il comando), non passa il percorso, ma il nome dell'eseguibile (come primo argomento)  
            perror(*cline);
            exit(1);
        }
        
        if(pid > 0){ //Se siamo nel processo padre (cioè la shell)
            sigaction(SIGINT, &sa_ign, NULL); //Viene messa come prima della wait
            
            //wait che attende il figlio, viene cancellata la entry della tabella dei processi riguardante il comando che e stato lanciato
            ret = waitpid(pid, &exitstat, 0);//il padre fa la wait e si ferma aspettando il processo con pid pid con nessuna opzione ed è in foreground
            
            if(ret == -1) 
                perror("Errore");
            
            //Verifica se il processo figlio è terminato a causa di un segnale
            if(WIFSIGNALED(ret) == 1) 
                printf("Processo terminato da un segnale\n");
        }
    }

    else/*if(where == BACKGROUND)*/{ //Se shell e comando sono eseguiti in parallelo (contemporaneamente)
        
        if (pid == (pid_t) 0){ //Se siamo nel processo figlio (cioè il comando)
            sigaction(SIGINT, &sa, NULL);
                    
            if(bandiera == 1){
                fd = creat(f, S_IRWXU); //fd = open(f, O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU); 
                if(fd == -1) //Se la creat restituisce un errore
                    exit(1);
                
                close(1); //Libera(svuota) la variabile 1 nella tabella dei file aperti del processo che si riferisce allo standard output
                dup(fd); //La dup mette ciò che si passa come argomento all'interno della variabile vuota con intero inferiore nella tabella file aperti del processo
                //dup2(fd, 1);
                close(fd);
            }
            
            execvp(*cline, cline);//execvp(percorso dell'eseguibile da sostituire primo termine, ambiente del processo da eseguire, cioè il percorso dell' eseguibile)
            perror(*cline);
            exit(1);
        }

        if(pid > 0){ //Se siamo nel processo padre (cioè la shell)
            sigaction(SIGINT, &sa_ign, NULL);
        }
        
    }
    
    sigaction(SIGINT, &sa, NULL);
}


int main(){
    while(userin(prompt) != EOF)
        procline();
}
