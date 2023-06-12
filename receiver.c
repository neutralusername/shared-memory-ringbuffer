#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>


int main(int argc, char *argv[]){



    if (argc != 2) {
        printf("expected 1 argument(ringbuffer size), got %d\n", argc - 1);
        exit(1);
    }
    int bufferSize = atoi(argv[1]);
    if (bufferSize < 1) {
        printf("expected positive integer, got %d\n", bufferSize);
        exit(1);
    }





    key_t receiverKey = ftok("receiver", bufferSize);
    if (receiverKey == -1) {
        printf("ftok failed\n");
        exit(1);
    }
    key_t bufferKey = ftok("buffer", bufferSize);
    if (bufferKey == -1) {
        printf("ftok failed\n");
        exit(1);
    }
    if (bufferKey == receiverKey) {
        printf("bufferKey and receiverKey are the same\n");
        exit(1);
    }





    int receiverSemId = semget(receiverKey, 1, IPC_EXCL | IPC_CREAT | 0666 );
    if (receiverSemId == -1) { 
        if (errno == EEXIST) { //semaphore for receiver already exists
            receiverSemId = semget(receiverKey, 1, 0666); //get the semaphore
        } else {
            printf("semget failed\n");
            exit(1);
        }
    } else { //if receiver was just created Set its value to 1 so that someone can start sending
        semctl(receiverSemId, 0, SETVAL, 1);
    }





    struct sembuf sembufLock = {.sem_num = 0, .sem_op = -1, .sem_flg = IPC_NOWAIT};
    if (semop(receiverSemId, &sembufLock, 1) == -1) {
        printf("semop lock failed\n");
        exit(1);
    }





    int receiverIndexShmId = shmget(receiverKey, sizeof(int), IPC_EXCL | IPC_CREAT | 0666);
    if (receiverIndexShmId == -1) {
        if (errno == EEXIST) { //receiverIndex already exists
            receiverIndexShmId = shmget(receiverKey, sizeof(int), 0666); //get the receiverIndex
        } else {
            printf("shmget failed\n");
            exit(1);
        }
    } else { //if receiverIndex was just created Set its value to 0 so that someone can start sending
        int *receiverIndex = shmat(receiverIndexShmId, NULL, 0);
        *receiverIndex = 0;
        shmdt(receiverIndex);
    } 
    int *receiverIndex = shmat(receiverIndexShmId, NULL, 0);





    int bufferShmId = shmget(bufferKey, bufferSize * sizeof(char), IPC_EXCL | IPC_CREAT | 0666);
    if (bufferShmId == -1) {
        if (errno == EEXIST) { //buffer already exists
            bufferShmId = shmget(bufferKey, bufferSize * sizeof(char), 0666); //get the buffer
        } else {
            printf("shmget failed\n");
            exit(1);
        }
    } else { //if buffer was just created Set its value to 0 so that someone can start sending
        char *buffer = shmat(bufferShmId, NULL, 0);
        for (int i = 0; i < bufferSize; i++) {
            buffer[i] = 0;
        }
        shmdt(buffer);
    } 
    char *buffer = shmat(bufferShmId, NULL, 0);





    int bufferSemId = semget(bufferKey, 1, IPC_EXCL | IPC_CREAT | 0666 );
    if (bufferSemId == -1) { 
        if (errno == EEXIST) { 
            bufferSemId = semget(bufferKey, 1, 0666); 
        } else {
            printf("semget failed\n");
            exit(1);
        }
    } else { 
        semctl(bufferSemId, 0, SETVAL, bufferSize); //number of free spaces in buffer
    }


   






    struct sembuf sembufUnlock = {.sem_num = 0, .sem_op = 1, .sem_flg = 0};
    if (semop(receiverSemId, &sembufUnlock, 1) == -1) {
        printf("semop unlock failed\n");
        exit(1);
    }



   
    return 0;
}
