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
    key_t senderKey2 = ftok("sender2", bufferSize);
    if (senderKey2 == -1) {
        printf("ftok failed\n");
        exit(1);
    }
    key_t receiverKey2 = ftok("receiver2", bufferSize);
    if (receiverKey2 == -1) {
        printf("ftok failed\n");
        exit(1);
    }





    int receiverLockSemId = semget(receiverKey, 1, IPC_EXCL | IPC_CREAT | 0666 );
    if (receiverLockSemId == -1) { 
        if (errno == EEXIST) { 
            receiverLockSemId = semget(receiverKey, 1, 0666); 
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





    int receiverIndexShmId = shmget(receiverKey, sizeof(int), IPC_EXCL | IPC_CREAT | 0666);
    if (receiverIndexShmId == -1) {
        if (errno == EEXIST) { 
            receiverIndexShmId = shmget(receiverKey, sizeof(int), 0666);
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





    int bufferShmId = shmget(bufferKey, bufferSize * sizeof(int), IPC_EXCL | IPC_CREAT | 0666);
    if (bufferShmId == -1) {
        if (errno == EEXIST) { 
            bufferShmId = shmget(bufferKey, bufferSize * sizeof(int), 0666); 
        } else {
            printf("shmget failed\n");
            exit(1);
        }
    } else { 
        int *buffer = shmat(bufferShmId, NULL, 0);
        for (int i = 0; i < bufferSize; i++) {
            buffer[i] = 0;
        }
        shmdt(buffer);
    } 
    int *buffer = shmat(bufferShmId, NULL, 0);


    


    int senderSemId2 = semget(senderKey2, 1, IPC_EXCL | IPC_CREAT | 0666 );
    if (senderSemId2 == -1) { 
        if (errno == EEXIST) { 
            senderSemId2 = semget(senderKey2, 1, 0666); 
        } else {
            printf("semget failed\n");
            exit(1);
        }
    } else { 
        semctl(senderSemId2, 0, SETVAL, bufferSize);
    }
    int receiverSemId2 = semget(receiverKey2, 1, IPC_EXCL | IPC_CREAT | 0666 );
    if (receiverSemId2 == -1) { 
        if (errno == EEXIST) { 
            receiverSemId2 = semget(receiverKey2, 1, 0666); 
        } else {
            printf("semget failed\n");
            exit(1);
        }
    } else { 
        semctl(receiverSemId2, 0, SETVAL, 0); 
    }




   

    while(1) {
        struct sembuf sembufLock = {.sem_num = 0, .sem_op = -1, .sem_flg = 0};
        if (semop(receiverSemId2, &sembufLock, 1) == -1) {
            printf("semop lock failed\n");
            exit(1);
        }

        int c = buffer[*receiverIndex];
        if (c == EOF) {
            break;
        }

        *receiverIndex = (*receiverIndex + 1) % bufferSize;
        printf("%c", c);
        fflush(stdout);

        struct sembuf sembufUnlock = {.sem_num = 0, .sem_op = 1, .sem_flg = 0};
        if (semop(senderSemId2, &sembufUnlock, 1) == -1) {
            printf("semop unlock failed\n");
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
    semctl(receiverSemId2, 0, IPC_RMID, 0);
   
    return 0;
}
