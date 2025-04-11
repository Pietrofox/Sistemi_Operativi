#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>

const char delimiters[] = " ";
sem_t *semaphores[256];
const char fileName[] = "catalog.txt";
//inzializazione prototipi
void initializeSemaphores();
void listen();
void view();
void release(char *car);
void lock(char *car);
int status(int i);
FILE * openFile();



int main(void) {
    initializeSemaphores();
    listen();
    for (int i = 0; semaphores[i] != NULL; i++) {
        sem_close(semaphores[i]);
    }
    return 0;
}

void initializeSemaphores() {  //0 = free, 1 = lock
    FILE *file = fopen(fileName, "r");
    if (file == NULL) {
        printf("Impossibile aprire il file\n");
    }
    char buffer[1024];
    int i = 0;
    while (fgets(buffer, 1024, file) != NULL) {
        // Rimuovi il newline dal buffer
        buffer[strcspn(buffer, "\n")] = '\0';

        // Alloca memoria per il nome del semaforo
        char *semaphoreName = malloc(strlen(buffer) + 2); // +2 per "/" e il terminatore null
        if (semaphoreName == NULL) {
            printf("Errore nell'allocazione della memoria\n");
        }

        // Costruisci il nome del semaforo
        strcpy(semaphoreName, "/");
        strcat(semaphoreName, buffer);

        // Apri o crea il semaforo e assegnalo all'array di semafori
        semaphores[i] = sem_open(semaphoreName, O_CREAT, 0777, 0);
        if (semaphores[i] == SEM_FAILED) {
            perror("sem_open failed");
            exit(EXIT_FAILURE);
        }
        
        free(semaphoreName);// Libera la memoria allocata per il nome del semaforo
        i++;
    }
    fclose(file);
}

void listen() {//funzione che si occupa di leggere il file e di assegnare i semafori
    while (1) { //loop infinito
        char input[100];
        printf("Command: ");//gestione input
        if (fgets(input, sizeof(input), stdin) != NULL) {//legge usando fgets e memorizza in array
            input[strcspn(input, "\n")] = '\0'; // Rimuovi il newline finale

            char *token;
            token = strtok(input, delimiters); //rende l'input un token
            if (token == NULL) {
                printf("No command entered\n");
                continue;
            }
            if (strcmp(token, "view") == 0) {
                view();//chiama la funzione view per visualizzare il catalogo
            } else if (strcmp(token, "release")==0){
                release(strtok(NULL, delimiters));//chiama la funzione release per liberare una auto
            } else if (strcmp(token, "lock") == 0){
                lock(strtok(NULL, delimiters));//chiama la funzione lock per bloccare l'auto
            } else if (strcmp(token, "quit") == 0) {
                return;//chiama la funzione quit per uscire dal programma
            } else {
                printf("Unknown Command\n");//gestisce se il comando non è riconosciuto
            }
        }
    }
}

void view(){//funzione per visionare le auto in catalog.txt
    FILE *file = openFile();

    char buffer[1024];
    int i = 0;
    while (fgets(buffer, 1024, file) != NULL) {
        const char * carStatus;
        int statusIndex = status(i);//inserisce il valore del semaforo in statusIndex
        if(statusIndex==1) {
            carStatus = "busy";
        }
        else if(statusIndex==0){
            carStatus = "free";
            }
        else {
            printf("Errore nel semaforo %d\n",i);//gestisce gli errori
            fclose(file);
            return;
        }
        buffer[strcspn(buffer, "\n")] = '\0';
        printf("Car: %s, status: %s \n",buffer, carStatus);
        i++;
    }
    fclose(file);
}


void release(char *car){//funzione per liberare un auto
    FILE *file = openFile();

    char buffer[1024];
    int i = 0;

    while (fgets(buffer, 1024, file) != NULL) {
       buffer[strcspn(buffer, "\n")] = '\0';
        if(strcmp(car, buffer) == 0){
            //printf("%s uguale a %s", car, buffer);
            if(status(i) == 0){
                printf("Error. Car: %s already free\n", car);
            } else{
                sem_wait(semaphores[i]);
                printf("Car: %s is now free\n", car);
            }
            fclose(file);
            return;
        }
        else
            //printf("%s diverso da %s", car, buffer);
        i++;
    }
    printf("Cannot find car %s\n", car);//gestisce l'errore
    fclose(file);
}


void lock(char *car){//funzione per bloccare un auto
    FILE * file = openFile();
    char buffer[1024];
    int i = 0;

    while (fgets(buffer, 1024, file) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';
        if(strcmp(car, buffer) == 0){
            if(status(i) == 1){
                printf("Error. Car %s already locked\n", car);//gestisce l'errore se già bloccata
            } else{
                sem_post(semaphores[i]);
                printf("Car: %s is now locked\n", car);//blocca l'auto
            }
            fclose(file);
            return;
        }
        i++;
    }
    printf("Cannot find car %s\n", car);//gestisce l'errore
    fclose(file);
}


int status(int i){
    if (semaphores[i] == NULL) {
        fprintf(stderr, "Error: NULL semaphore pointer\n");
        return -1; // valore che indica errore
    }

    int sval;//valore del semaforo
    if (sem_getvalue(semaphores[i], &sval) != 0) {
        perror("sem_getvalue");
        return -1; // valore che indica errore
    }
    return sval;
}


FILE * openFile(){
    FILE *file = fopen("catalog.txt", "r");//apre catalog.txt in mod. lettura
    if (file == NULL) {
        printf("Impossibile aprire il file\n");//gestisce l'errore
    }
    return file;
}