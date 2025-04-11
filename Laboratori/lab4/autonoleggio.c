#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/file.h>

#define MAX_CAR_NAME_LENGTH (20)
#define MAX_CARS (3)
#define FILENAME "catalog.txt"

typedef struct {
    char name[MAX_CAR_NAME_LENGTH];
    int status; // 0 = free, 1 = busy
    sem_t *semaphore;
} Car;

Car cars[MAX_CARS];

void lock_file(int fd) {
    if (flock(fd, LOCK_EX) == -1) {
        perror("flock LOCK_EX");
        exit(1);
    }
}

void unlock_file(int fd) {
    if (flock(fd, LOCK_UN) == -1) {
        perror("flock LOCK_UN");
        exit(1);
    }
}

void init_cars() {
    int fd = open(FILENAME, O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        exit(1);
    }
    

    FILE *file = fopen(FILENAME, "r");
    if (file == NULL) {
        perror("Error converting file descriptor to FILE*");
        close(fd);
        exit(1);
    }

    for (int i = 0; i < MAX_CARS; i++) {
        if (fscanf(file, "%s %ls", cars[i].name, &cars[i].status) != 2) {
            printf("Error reading from file\n");
            fclose(file);
            exit(1);
        }

        char sem_name[MAX_CAR_NAME_LENGTH + 10];
        sprintf(sem_name, "%s_semaphore", cars[i].name);
        cars[i].semaphore = sem_open(sem_name, O_CREAT, 0644, 1);
        if (cars[i].semaphore == SEM_FAILED) {
            perror("sem_open failed");
            fclose(file);
            exit(1);
        }
    }

    fclose(file); // Also unlocks the file
}

void save_cars_state() {
    int fd = open(FILENAME, O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        exit(1);
    }

    lock_file(fd);

    FILE *file = fdopen(fd, "w");
    if (file == NULL) {
        perror("Error converting file descriptor to FILE*");
        close(fd);
        exit(1);
    }

    for (int i = 0; i < MAX_CARS; i++) {
        fprintf(file, "%s %d\n", cars[i].name, cars[i].status);
    }

    fclose(file); // Also unlocks the file
}

void view() {
    int fd = open(FILENAME, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        exit(1);
    }

    lock_file(fd);

    FILE *file = fdopen(fd, "r");
    if (file == NULL) {
        perror("Error converting file descriptor to FILE*");
        close(fd);
        exit(1);
    }

    for (int i = 0; i < MAX_CARS; i++) {
        if (fscanf(file, "%s %d", cars[i].name, &cars[i].status) != 2) {
            printf("Error reading from file\n");
            fclose(file);
            exit(1);
        }
        printf("Car: %s, status: %s\n", cars[i].name, cars[i].status == 0 ? "free" : "busy");
    }

    fclose(file); // Also unlocks the file
}

void lock_car(const char *car_name) {
    for (int i = 0; i < MAX_CARS; i++) {
        if (strcmp(cars[i].name, car_name) == 0) {
            if (sem_wait(cars[i].semaphore) == 0) {
                if (cars[i].status == 0) {
                    cars[i].status = 1; // Mark as locked
                    printf("Car: %s is now locked\n", cars[i].name);
                    save_cars_state();
                } else {
                    printf("Error. Car: %s already locked\n", car_name);
                }
                sem_post(cars[i].semaphore);
            } else {
                perror("sem_wait failed");
            }
            return;
        }
    }
    printf("Cannot find car: %s\n", car_name);
}

void release_car(const char *car_name) {
    for (int i = 0; i < MAX_CARS; i++) {
        if (strcmp(cars[i].name, car_name) == 0) {
            if (sem_wait(cars[i].semaphore) == 0) {
                if (cars[i].status == 1) {
                    cars[i].status = 0; // Mark as free
                    printf("Car: %s is now free\n", car_name);
                    save_cars_state();
                } else {
                    printf("Error. Car: %s already free\n", car_name);
                }
                sem_post(cars[i].semaphore);
            } else {
                perror("sem_wait failed");
            }
            return;
        }
    }
    printf("Cannot find car: %s\n", car_name);
}

int main() {
    init_cars();

    // Main program loop
    char command[100];
    while (1) {
        printf("\nCommand:");
        if (fgets(command, sizeof(command), stdin) == NULL) {
            perror("Error reading command");
            break;
        }

        command[strcspn(command, "\n")] = 0; // Remove trailing newline

        if (strcmp(command, "view") == 0) {
            view();
        } else if (strncmp(command, "lock ", 5) == 0) {
            char car[MAX_CAR_NAME_LENGTH];
            sscanf(command + 5, "%s", car);
            lock_car(car);
        } else if (strncmp(command, "release ", 8) == 0) {
            char car[MAX_CAR_NAME_LENGTH];
            sscanf(command + 8, "%s", car);
            release_car(car);
        } else if (strcmp(command, "quit") == 0) {
            break;
        } else {
            printf("Unknown Command\n");
        }
    }

    save_cars_state(); // Save the car states before exiting

    // Close and unlink semaphores
    for (int i = 0; i < MAX_CARS; i++) {
        sem_close(cars[i].semaphore);
        char sem_name[MAX_CAR_NAME_LENGTH + 10];
        sprintf(sem_name, "%s_semaphore", cars[i].name);
        sem_unlink(sem_name);
    }

    return 0;
}
