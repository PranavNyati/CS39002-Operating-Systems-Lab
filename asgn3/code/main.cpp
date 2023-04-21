#include<iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <set>
#include <unistd.h>
#include <wait.h>
#include <string.h>

using namespace std;

#define FILEPATH "facebook_combined.txt"
#define SHM_EDGE_SZ 150000


int main(int argc, char *argv[]){

    int optim_flag = 0;
    if (argc == 2){
        if (strcmp(argv[1], "-optimize") == 0){
            optim_flag = 1;
        }
    }

    // create shared memory for storing the graph as a set of edges
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

    // attach to the shared memory
    int *shm1 = (int*) shmat(shmid1, NULL, 0);
    if (shm1 == (int*) -1){
        perror("Error while attaching to shared memory.\n");
        exit(EXIT_FAILURE);
    }

    int *shm2 = (int*) shmat(shmid2, NULL, 0);
    if (shm2 == (int*) -1){
        perror("Error while attaching to shared memory.\n");
        exit(EXIT_FAILURE);
    }

    // open the text file to read the graph and store it in the shared memory
    FILE *fp = fopen(FILEPATH, "r");
    if (fp == NULL){
        perror("Error while opening the file.\n");
        exit(EXIT_FAILURE);
    }

    // read the file line by line
    char line[50];
    int num_edges = 0, num_nodes = 0, node1, node2;
  
    // create a set of nodes
    set<int> nodes;

    shm1[0] = num_nodes; 
    shm1[1] = num_edges;
    shm2[0] = num_nodes;
    shm2[1] = num_edges;

    while(fgets(line, sizeof(line), fp) != NULL){
        sscanf(line, "%d %d", &node1, &node2);

        nodes.insert(node1); nodes.insert(node2);

        num_edges++;
        shm1[num_edges + 1] = node1;
        shm2[num_edges + 1] = node2;

        // update the number of nodes
        num_nodes = nodes.size();
        shm1[0] = num_nodes;
        shm2[0] = num_nodes;

        // update the number of edges
        shm1[1] = num_edges;
        shm2[1] = num_edges;
    }
        
    fclose(fp);
    printf("Number of nodes: %d, Number of edges: %d\n", num_nodes, num_edges);

    shmdt(shm1);    // detach from the shared memory
    shmdt(shm2);

    // fork the producer and consumer processes
    pid_t producer_pid = fork();
    if (producer_pid == 0){
        execl("./prod", "./prod", NULL);
    }

    else{
        pid_t consumers_pid[10];

        // fork 10 consumer processes
        for (int i = 0; i < 10; i++){
            consumers_pid[i] = fork();
            char *cons_idx = (char*) malloc(sizeof(char));
            sprintf(cons_idx, "%d", i);
            if (optim_flag == 0){
                if (consumers_pid[i] == 0){
                    
                    execl("./cons", "./cons", cons_idx, NULL);
                }
            }
            else if (optim_flag == 1){
                if (consumers_pid[i] == 0){
                    execl("./cons", "./cons", cons_idx, "-o", NULL);
                }
            }

        }

        // block the parent process until all the child processes are done
        waitpid(producer_pid, NULL, 0);

        for (int i = 0; i < 10; i++){
            waitpid(consumers_pid[i], NULL, 0);
        }

        // delete the shared memory
        shmctl(shmid1, IPC_RMID, NULL);
        shmctl(shmid2, IPC_RMID, NULL);
    }

    return 0;   
}