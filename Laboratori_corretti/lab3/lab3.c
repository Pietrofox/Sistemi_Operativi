#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXPROCESSES 256
#define MAXLINE 50

// Dichiarazione Funzioni
int estraiParametri(const char *fileName, char *output[]);

void creaComandi(char comandi[], char *parametri[], char *comandiCompleti[]);

void eseguiComandi( char *comandiDaEseguire[],int numeroProcessi);

int main(int argc, const char *argv[]) {
    if(argc!=4)
        return -1;
    char *parametri[1024];
    if (estraiParametri(argv[1], parametri) == -1) {
        return 0;
    }

    char *comandiCompleti[1024];
    char comandi[1024];
    strcpy(comandi, argv[3]);
    creaComandi(comandi, parametri, comandiCompleti);

    char *endptr;

    eseguiComandi(comandiCompleti, (int)strtol(argv[2], &endptr, 10));
    return 0;
}


int estraiParametri(const char *fileName, char *output[]) {
    FILE *file = fopen(fileName, "r"); //apertura file

    if (file == NULL) {
        printf("Impossibile aprire il file\n");
        return -1; //gestione errore se file vuoto
    }

    char buffer[1024];
    int i = 0;
    while (fgets(buffer, 1024, file) != NULL) {
        output[i] = malloc(strlen(buffer) + 1);
        strcpy(output[i], buffer);
        if (output[i][strlen(output[i]) - 1] == '\n') {   //rimuovere \n alla fine dei parametri
            output[i][strlen(output[i]) - 1] = '\0';
        }
        i++;
    }
    output[i] = NULL;
    fclose(file);
    return 0;
}

void creaComandi(char comandi[], char *parametri[], char *comandiCompleti[]) {

    const char delimitatore[] = "%";
    int i = 0;
    while (parametri[i] != NULL) {
        char stringaProvvisoria[100];
        strcpy(stringaProvvisoria, comandi);
        comandiCompleti[i] = malloc(strlen(comandi) + strlen(parametri[i]) + 1);
        strcpy(comandiCompleti[i], strtok(stringaProvvisoria, delimitatore));
        strcat(comandiCompleti[i], parametri[i]);
        strcat(comandiCompleti[i], strtok(NULL, delimitatore));
        if (comandiCompleti[i][strlen(comandiCompleti[i]) - 1] == '\n') {   //rimuovere \n alla fine dei parametri
            comandiCompleti[i][strlen(comandiCompleti[i]) - 1] = '\0';
        }
        i++;
    }
    comandiCompleti[i] = NULL;
} //funzione che crea i comandi da eseguire

void eseguiComandi(char *comandiDaEseguire[], int const numeroProcessi) {

    int pipes[MAXPROCESSES][2];
    int numeroDiOperazioni = 0;
    int i = 0;   //trovo quante operazione deve fare ogni processo
    while (comandiDaEseguire[i] != NULL) {
        numeroDiOperazioni += 1;
        i++;
    }

    int numeroDiOperazioniPerProcesso = (int) (numeroDiOperazioni / numeroProcessi);

    char buffer[MAXLINE];
    int contatoreComandi = 0;

    for (i = 0; i < numeroProcessi; ++i) {

        pipe(pipes[i]);
        if (fork() > 0) { /* Padre */
            close(pipes[i][0]);
        } else { /* Figlio */
            close(pipes[i][1]);
            while (read(pipes[i][0], buffer, MAXLINE) > 0) {

                strcat(buffer,"\n");
                //system(buffer);
                printf("%s", buffer);
            }
            close(pipes[i][0]);

            exit(0);
        }
//qua non gira, zio billy
        int j = 0;
        while (j <= numeroDiOperazioniPerProcesso && comandiDaEseguire[contatoreComandi] != NULL) {
            write(pipes[i][1], comandiDaEseguire[contatoreComandi], MAXLINE);
            j++;
            contatoreComandi++;
        }
        close(pipes[i][1]);
    }
    for (i = 0; i < numeroProcessi; ++i) {
        wait(NULL);
    }
} //funzione che esegue i comandi passati come argomento