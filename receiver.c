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





    key_t receiverLockKey = ftok("receiver", bufferSize);
    if (receiverLockKey == -1) {
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





    int receiverLockSemId = semget(receiverLockKey, 1, IPC_EXCL | IPC_CREAT | 0666 );
    if (receiverLockSemId == -1) { 
        if (errno == EEXIST) { 
            receiverLockSemId = semget(receiverLockKey, 1, 0666); 
        } else {
            printf("semget failed\n");
            exit(1);
        }
    } else { 
        semctl(receiverLockSemId, 0, SETVAL, 1);
    }





    struct sembuf sembufLock = {.sem_num = 0, .sem_op = -1, .sem_flg = IPC_NOWAIT};
    if (semop(receiverLockSemId, &sembufLock, 1) == -1) {
        printf("semop lock failed\n");
        exit(1);
    }





    int receiverIndexShmId = shmget(receiverLockKey, sizeof(int), IPC_EXCL | IPC_CREAT | 0666);
    if (receiverIndexShmId == -1) {
        if (errno == EEXIST) { 
            receiverIndexShmId = shmget(receiverLockKey, sizeof(int), 0666);
        } else {
            printf("shmget failed\n");
            exit(1);
        }
    } else { 
        int *receiverIndex = shmat(receiverIndexShmId, NULL, 0);
        *receiverIndex = 0;
        shmdt(receiverIndex);
    } 
    int *receiverIndex = shmat(receiverIndexShmId, NULL, 0);





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




   

    while(1) {
        struct sembuf readableElementsDecrease = {.sem_num = 0, .sem_op = -1, .sem_flg = 0};
        if (semop(readableElementsSemId, &readableElementsDecrease, 1) == -1) {
            printf("semop readableElementsDecrease failed\n");
            exit(1);
        }

        int c = buffer[*receiverIndex];
        if (c == EOF) {
            break;
        }

        *receiverIndex = (*receiverIndex + 1) % bufferSize;
        printf("%c", c);
        fflush(stdout);

        struct sembuf writeableElementsIncrease = {.sem_num = 0, .sem_op = 1, .sem_flg = 0};
        if (semop(writeableElementsSemId, &writeableElementsIncrease, 1) == -1) {
            printf("semop writeableElementsIncrease failed\n");
            exit(1);
        }
    }



    struct sembuf sembufUnlock = {.sem_num = 0, .sem_op = 1, .sem_flg = 0};
    if (semop(receiverLockSemId, &sembufUnlock, 1) == -1) {
        printf("semop unlock failed\n");
        exit(1);
    }

    shmdt(receiverIndex);
    shmdt(buffer);

    semctl(receiverLockSemId, 0, IPC_RMID, 0);
    semctl(readableElementsSemId, 0, IPC_RMID, 0);

    shmctl(receiverIndexShmId, IPC_RMID, 0);
    shmctl(sharedMemoryRingbufferShmId, IPC_RMID, 0);
   
    return 0;
}
