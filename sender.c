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





    key_t senderLockKey = ftok("sender", bufferSize);
    if (senderLockKey == -1) {
        printf("ftok failed\n");
        exit(1);
    }
    key_t sharedMemoryRingbufferKey = ftok("buffer", bufferSize);
    if (sharedMemoryRingbufferKey == -1) {
        printf("ftok failed\n");
        exit(1);
    }
    key_t writeableElementsKey = ftok("sender2", bufferSize);
    if (writeableElementsKey == -1) {
        printf("ftok failed\n");
        exit(1);
    }
    key_t readableElementsKey = ftok("receiver2", bufferSize);
    if (readableElementsKey == -1) {
        printf("ftok failed\n");
        exit(1);
    }





    int senderLockSemId = semget(senderLockKey, 1, IPC_EXCL | IPC_CREAT | 0666 );
    if (senderLockSemId == -1) { 
        if (errno == EEXIST) { 
            senderLockSemId = semget(senderLockKey, 1, 0666); 
        } else {
            printf("semget failed\n");
            exit(1);
        }
    } else { 
        semctl(senderLockSemId, 0, SETVAL, 1);
    }





    struct sembuf sembufLock = {.sem_num = 0, .sem_op = -1, .sem_flg = IPC_NOWAIT};
    if (semop(senderLockSemId, &sembufLock, 1) == -1) {
        printf("semop lock failed\n");
        exit(1);
    }





    int senderIndexShmId = shmget(senderLockKey, sizeof(int), IPC_EXCL | IPC_CREAT | 0666);
    if (senderIndexShmId == -1) {
        if (errno == EEXIST) { 
            senderIndexShmId = shmget(senderLockKey, sizeof(int), 0666); //get the senderIndex
        } else {
            printf("shmget failed\n");
            exit(1);
        }
    } else { 
        int *senderIndex = shmat(senderIndexShmId, NULL, 0);
        *senderIndex = 0;
        shmdt(senderIndex);
    } 
    int *senderIndex = shmat(senderIndexShmId, NULL, 0);





    int sharedMemoryRingbufferShmId = shmget(sharedMemoryRingbufferKey, bufferSize * sizeof(int), IPC_EXCL | IPC_CREAT | 0666);
    if (sharedMemoryRingbufferShmId == -1) {
        if (errno == EEXIST) { 
            sharedMemoryRingbufferShmId = shmget(sharedMemoryRingbufferKey, bufferSize * sizeof(int), 0666); 
        } else {
            printf("shmget failed\n");
            exit(1);
        }
    } else { 
        int *buffer = shmat(sharedMemoryRingbufferShmId, NULL, 0);
        for (int i = 0; i < bufferSize; i++) {
            buffer[i] = 0;
        }
        shmdt(buffer);
    } 
    int *buffer = shmat(sharedMemoryRingbufferShmId, NULL, 0);





    int writeableElementsSemId = semget(writeableElementsKey, 1, IPC_EXCL | IPC_CREAT | 0666 );
    if (writeableElementsSemId == -1) { 
        if (errno == EEXIST) { 
            writeableElementsSemId = semget(writeableElementsKey, 1, 0666); 
        } else {
            printf("semget failed\n");
            exit(1);
        }
    } else { 
        semctl(writeableElementsSemId, 0, SETVAL, bufferSize); 
    }
    int readableElementsSemId = semget(readableElementsKey, 1, IPC_EXCL | IPC_CREAT | 0666 );
    if (readableElementsSemId == -1) { 
        if (errno == EEXIST) { 
            readableElementsSemId = semget(readableElementsKey, 1, 0666); 
        } else {
            printf("semget failed\n");
            exit(1);
        }
    } else { 
        semctl(readableElementsSemId, 0, SETVAL, 0); 
    }





    int c;
    while((c = getchar()) != EOF) {
        struct sembuf writeableElementsDecrease = {.sem_num = 0, .sem_op = -1, .sem_flg = 0};
        if (semop(writeableElementsSemId, &writeableElementsDecrease, 1) == -1) {
            printf("semop writeableElementsDecrease failed\n");
            exit(1);
        }
        buffer[*senderIndex] = c;
        *senderIndex = (*senderIndex + 1) % bufferSize;
        struct sembuf readableElementsIncrease = {.sem_num = 0, .sem_op = 1, .sem_flg = 0};
        if (semop(readableElementsSemId, &readableElementsIncrease, 1) == -1) {
            printf("semop readableElementsIncrease failed\n");
            exit(1);
        }
    }



    buffer[*senderIndex] = EOF;
    struct sembuf sembuf2 = {.sem_num = 0, .sem_op = 1, .sem_flg = 0};
    if (semop(readableElementsSemId, &sembuf2, 1) == -1) {
        printf("semop eof failed\n");
        exit(1);
    }


    struct sembuf sembufUnlock = {.sem_num = 0, .sem_op = 1, .sem_flg = 0};
    if (semop(senderLockSemId, &sembufUnlock, 1) == -1) {
        printf("semop unlock failed\n");
        exit(1);
    }

    shmdt(senderIndex);
    shmdt(buffer);  

    semctl(senderLockSemId, 0, IPC_RMID, 0);
    semctl(writeableElementsSemId, 0, IPC_RMID, 0);

    return 0;
}
