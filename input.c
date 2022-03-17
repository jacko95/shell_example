#include "smallsh.h"

/* buffers per la riga di input e la sua segmentazione in "tokens";
puntatori per scorrere i buffers */

static char inpbuf[MAXBUF];//In inpbuf (array caratteri) viene parcheggia la riga in input
static char tokbuf[2*MAXBUF]; //Array di carateri in cui viene copia il contenuto di inpbuf, ma con al posto degli spazi degli \0. (contenuto i inpuf tokenizzato)
static char* ptr; //ptr: puntatore al carattere successivo da leggere all' interno di inbuf
static char* tok; //tok: puntatore al carattere successivo da leggere all' interno di tokbuf

/* array di caratteri che hanno una interpretazione "speciale" 
nei comandi */
static char special[] = {' ', '\t', '&', ';', '\n', '>', '\0'};

int userin(char *p)/* stampa il prompt e legge una riga */
{
    int c;
    int count; //Indice che scorre inbuf

    /* inizializzazioni per altre routines */

    ptr = inpbuf; //ptr (puntatore a static char) che punta a inpbuf
    tok = tokbuf; //tok (puntatore a static char) che punta a tokbuf

    /* stampa il prompt */
    printf("%s ", p);

    count = 0;

    while(1) {

        if ((c = getchar()) == EOF)
            return(EOF);

        //Se non si è superato il massimo dei caratteri che possono stare all'interno della riga in input, si copia il carattere letto alla posizione successiva di count in inpbuf
        if (count < MAXBUF)
            inpbuf[count++] = c;

        /* se si legge il newline, la riga in input e' finita */
        if (c == '\n' && count < MAXBUF) {
            inpbuf[count] = '\0';
            return(count); //Restituisce l'indice in cui si trova lo \0 nell'array inpbuf, quindi il carattere successivo all'ultimo comune in tutte le stringhe di caratteri 
        }

        //Caso in cui fosse stato superato il massimo di caratteri consentiti nella riga in input
        if (c == '\n') {/* implicito se si arriva qui: count >= MAXBUF */
            printf("riga in input troppo lunga\n");
            count = 0; //Si azzera count  e si va a leggere una nuova riga
            printf("%s ", p);
        }
    }
}

int gettok(char **outptr){ //gettok prende in ingresso un puntatore all' array di caratteri che contiene le stringhe della riga in input
    int type;

    /* si piazza *outptr in modo che punti al primo byte dove si cominicera' a scrivere il simbolo letto */  
    *outptr = tok;

    /* salta eventuali spazi in inpbuf */
    while (*ptr == ' ' || *ptr == '\t') ptr++;

    /* copia il primo carattere del simbolo */
    *tok++ = *ptr;

    /* a seconda del carattere decide il tipo di simbolo */
    switch(*ptr++){

        case '\n':
            type = EOL; break;
        case '&':
            type = AMPERSAND; break;
        case ';':
            type = SEMICOLON; break;
        case '>':
            type = MAGGIORE; break;
        default:
            type = ARG;// Se non riconosce i caratteri speciali, allora e' un argomeno
      
        /* copia gli altri caratteri del simbolo */
        while(inarg(*ptr))//Finchè il contenuto di ptr passato a inarg restituisce restituisce 1, quindi finchè è diverso da un carattere speciale, 
            *(tok++) = *ptr++;//Metti il carattere succesivo in inbuf all'interno di tokbuf. Il ++ messo dopo tok la variabile viene aumentata successovamente a questa istruzione, se prima parte già incrementata
    }

    /* aggiunge \0 al fondo */
    *tok++ = '\0';//Si aggiunge \0 alla fine di ogni ????
    
    return(type);

}

/* verifica se c non e' un carattere speciale */
int inarg(char c){
    char *wrk;

    for (wrk = special; *wrk != '\0'; wrk++) //wrk è un carattere che punta al primo elemento dell'array di caratteri speciali special
        if (c == *wrk) return(0); //Lo scorre finchè non arriva al carattere in input (dereferenziando wrk)

    return(1);
}

