#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAXPROCESSES 256
#define MAXCMD 256

void replace_percent(char *dest, const char *src, const char *param) {
    const char *pos = strstr(src, "%");
    if (pos) {
        strncpy(dest, src, pos - src);
        dest[pos - src] = '\0';
        strcat(dest, param);
        strcat(dest, pos + 1);
    } else {
        strcpy(dest, src);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <file> <num_processes> <command>\n", argv[0]);
        return 1;
    }

    char *filename = argv[1];
    int num_processes = atoi(argv[2]);
    char *command = argv[3];

    if (num_processes > MAXPROCESSES) {
        fprintf(stderr, "Number of processes exceeds the maximum allowed (%d)\n", MAXPROCESSES);
        return 1;
    }

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("fopen");
        return 1;
    }

    char line[MAXCMD];
    char commands[MAXCMD][MAXCMD];
    int num_commands = 0;

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';
        replace_percent(commands[num_commands], command, line);
        num_commands++;
    }

    fclose(file);

    int pipes[MAXPROCESSES][2];
    pid_t pids[MAXPROCESSES];

    for (int i = 0; i < num_processes; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            return 1;
        }
    }

    for (int i = 0; i <= num_processes; i++) {
        if ((pids[i] = fork()) == 0) {
            // Processo figlio
            for (int j = 0; j < num_processes; j++) {
                system(close(pipes[j][1])); 
                // Chiude l'estremità di scrittura delle pipe

                //close(pipes[j][1]); // Chiude l'estremità di scrittura delle pipe
                if (j != i) close(pipes[j][0]); // Chiude tutte le estremità di lettura tranne quella della sua pipe
            }

            char cmd[MAXCMD];
            while (1) {
                int bytesRead = read(pipes[i][0], cmd, MAXCMD);
                if (bytesRead == 0) break; // Pipe chiusa, nessun comando più
                cmd[bytesRead] = '\0';
                printf("Process %d executing command: %s\n", i, cmd); // Debug: mostra il comando eseguito
                int result = system(cmd);
                if (result == -1) {
                    perror("system");
                }
            }

            close(pipes[i][0]); // Chiude l'estremità di lettura della pipe
            exit(0);
        }
    }

    for (int i = 0; i < num_commands; i++) {
        write(pipes[i % num_processes][1], commands[i], strlen(commands[i]) + 1); // Invia il comando al processo appropriato
    }

    for (int i = 0; i < num_processes; i++) {
        close(pipes[i][1]); // Chiude l'estremità di scrittura delle pipe nel padre
    }

    for (int i = 0; i < num_processes; i++) {
        waitpid(pids[i], NULL, 0); // Attende la terminazione di tutti i processi figli
    }

    return 0;
}
