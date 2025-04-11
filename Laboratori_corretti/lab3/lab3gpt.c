#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAXPROCESSES 256
#define MAXLINE 1024

// Function declarations
int estraiParametriDaFile(const char *fileName, char *output[]);
void creaComandi(const char comandi[], char *parametri[], char *comandiCompleti[]);
void eseguiComandi(char *comandiDaEseguire[], int numeroProcessi);

int main(int argc, const char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <parameters_file> <num_processes> <command>\n", argv[0]);
        return -1;
    }

    char *parametri[1024];
    if (estraiParametriDaFile(argv[1], parametri) == -1) {
        return -1;
    }

    char *comandiCompleti[1024];
    creaComandi(argv[3], parametri, comandiCompleti);

    int numProcesses = atoi(argv[2]);
    if (numProcesses <= 0 || numProcesses > MAXPROCESSES) {
        fprintf(stderr, "Invalid number of processes. Must be between 1 and %d\n", MAXPROCESSES);
        return -1;
    }

    eseguiComandi(comandiCompleti, numProcesses);
    return 0;
}

// Extract parameters from a file and store them in the output array
int estraiParametriDaFile(const char *fileName, char *output[]) {
    FILE *file = fopen(fileName, "r");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    char buffer[MAXLINE];
    int i = 0;
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';  // Remove newline character
        output[i] = strdup(buffer);  // Duplicate buffer and assign to output[i]
        if (output[i] == NULL) {
            perror("Memory allocation error");
            fclose(file);
            return -1;
        }
        i++;
    }
    output[i] = NULL;  // Null-terminate the output array
    fclose(file);
    return 0;
}

// Create complete commands by substituting placeholders in the base command with parameters
void creaComandi(const char comandi[], char *parametri[], char *comandiCompleti[]) {
    int i = 0;
    while (parametri[i] != NULL) {
        comandiCompleti[i] = malloc(strlen(comandi) + strlen(parametri[i]) + 1);
        if (comandiCompleti[i] == NULL) {
            perror("Memory allocation error");
            exit(EXIT_FAILURE);
        }
        sprintf(comandiCompleti[i], comandi, parametri[i]);  // Construct the complete command
        i++;
    }
    comandiCompleti[i] = NULL;  // Null-terminate the commands array
}

// Execute the commands using the specified number of processes
void eseguiComandi(char *comandiDaEseguire[], int numeroProcessi) {
    int numCommands = 0;
    while (comandiDaEseguire[numCommands] != NULL) {
        numCommands++;
    }

    // Calculate how many commands each process should handle
    int commandsPerProcess = numCommands / numeroProcessi;
    int remainingCommands = numCommands % numeroProcessi;

    int pipes[MAXPROCESSES][2];
    for (int i = 0; i < numeroProcessi; ++i) {
        if (pipe(pipes[i]) == -1) {
            perror("Pipe creation error");
            exit(EXIT_FAILURE);
        }

        if (fork() == 0) {  // Child process
            close(pipes[i][1]);  // Close the write end of the pipe in the child
            char buffer[MAXLINE];
            while (read(pipes[i][0], buffer, MAXLINE) > 0) {
                buffer[strcspn(buffer, "\n")] = '\0';  // Remove newline character
                printf("Executing: %s\n", buffer);  // Print the command to be executed
                system(buffer);  // Execute the command
            }
            close(pipes[i][0]);  // Close the read end of the pipe in the child
            exit(0);
        } else {  // Parent process
            close(pipes[i][0]);  // Close the read end of the pipe in the parent

            int start = i * commandsPerProcess;
            int end = (i + 1) * commandsPerProcess + (i < remainingCommands ? 1 : 0);
            for (int j = start; j < end && comandiDaEseguire[j] != NULL; ++j) {
                write(pipes[i][1], comandiDaEseguire[j], strlen(comandiDaEseguire[j]) + 1);  // Write command to the pipe
            }
            close(pipes[i][1]);  // Close the write end of the pipe in the parent
        }
    }

    // Wait for all child processes to finish
    for (int i = 0; i < numeroProcessi; ++i) {
        wait(NULL);
    }
}
