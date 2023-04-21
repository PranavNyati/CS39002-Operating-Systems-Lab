#include<iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <set>
#include <unistd.h>
#include <wait.h>

#define SHM_EDGE_SZ 150000

using namespace std;


#define FILEPATH "facebook_combined.txt"

int main(){
    key_t key1 = ftok(FILEPATH, 37);
    int shmid1 = shmget(key1, SHM_EDGE_SZ*sizeof(int), IPC_CREAT | 0666);

    if (shmid1 == -1){
        perror("Error while creating shared memory.\n");
        exit(EXIT_FAILURE);
    }

    key_t key2 = ftok(FILEPATH, 38);
    int shmid2 = shmget(key2, SHM_EDGE_SZ*sizeof(int), IPC_CREAT | 0666);

    if (shmid2 == -1){
        perror("Error while creating shared memory.\n");
        exit(EXIT_FAILURE);
    }


    // delete the shared memory
    shmctl(shmid1, IPC_RMID, NULL);
    shmctl(shmid2, IPC_RMID, NULL);


    // also delete the 10 shared memory segments for the 10 consumers

    for (int i = 0; i < 10; i++){
        key_t key = ftok(FILEPATH, 40+i);
        int shmid = shmget(key, 100*sizeof(int), IPC_CREAT | 0666);

        if (shmid == -1){
            perror("Error while creating shared memory.\n");
            exit(EXIT_FAILURE);
        }

        shmctl(shmid, IPC_RMID, NULL);
    }

    return 0;
}