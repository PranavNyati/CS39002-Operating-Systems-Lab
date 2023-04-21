#include<iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <set>
#include <vector>
#include <algorithm>
#include <time.h>
#include <unistd.h>

using namespace std;

#define FILEPATH "facebook_combined.txt"
#define SHM_EDGE_SZ 150000

// sort by the second element of the pair in decreasing order
bool sort_by_sec_desc(const pair<int, int> &a, const pair<int, int> &b){
    return (a.second > b.second);
}

// function to calculate the degree of each of the existing nodes and sort them in decreasing order
// finally, returns a vector of pairs (node, degree) sorted in decreasing order of degree
// and a prob vector <pair<int, double>> which stores the probability of each node being selected
int calc_degree(int *shm1, int *shm2, int num_nodes, int num_edges, vector <pair<int, int>> &node_degree, vector <pair<int, double>> &node_prob){
    
    for (int i = 0; i < num_nodes; i++){
        node_degree.push_back(make_pair(i, 0));
    }

    for (int i = 0; i < num_edges; i++){
        node_degree[shm1[i+2]].second++;
        if (shm1[2 + i] != shm2[2 + i]){
            node_degree[shm2[i + 2]].second++;
        }
    }

    sort(node_degree.begin(), node_degree.end(), sort_by_sec_desc);       // sort the nodes in decreasing order of their degree
    
    // calculate the probability of each node being selected
    double degree_sum = 0;
    for (int i = 0; i < num_nodes; i++){
        degree_sum += node_degree[i].second;
    }

    for (int i = 0; i < num_nodes; i++){
        node_prob.push_back(make_pair(node_degree[i].first, node_degree[i].second/degree_sum));
    }

    // sort the nodes in decreasing order of their probability
    sort(node_prob.begin(), node_prob.end(), [](pair<int, double> a, pair<int, double> b){
        return a.second > b.second;
    });

    // return the cumulative sum of the probabilities
    for (int i = 1; i < num_nodes; i++){
        node_prob[i].second += node_prob[i-1].second;
    }
    return 0;
}

// function to map the newly added node to the 10 consumer processes using 10 shared memory segments
int map_nodes_to_cons(vector<int> &new_nodes){

    int num_new_nodes = new_nodes.size();
    int nodes_per_cons = num_new_nodes/10;
    int rem = num_new_nodes - (10*nodes_per_cons);

    // create 10 shared memory segments for storing the nodes mapped to each of the 10 consumers
    for (int i = 0; i < 10; i++){

        key_t key = ftok(FILEPATH, 40 + i);
        int shmid = shmget(key, 100*sizeof(int), IPC_CREAT | 0666);
        if (shmid == -1){
            perror("Error while creating shared memory.\n");
            exit(EXIT_FAILURE);
        }

        // attach to the shared memory
        int *shm = (int*) shmat(shmid, NULL, 0);
        if (shm == (int*) -1){
            perror("Error while attaching to shared memory.\n");
            exit(EXIT_FAILURE);
        }

        // store the nodes mapped to the ith consumer in the ith shared memory segment with the first element being the number of nodes
        if (i < rem)
            shm[0] = nodes_per_cons + 1;
        else 
            shm[0] = nodes_per_cons;

        for (int j = 0; j < shm[0]; j++){
            shm[j+1] = new_nodes[j];
        }
        // fill the remaining elements of the shared memory segment with -1
        for (int j = shm[0] + 1; j < 100; j++){
            shm[j] = -1;
        }
        
        // remove the nodes already mapped to the ith consumer from the new_nodes vector
        new_nodes.erase(new_nodes.begin(), new_nodes.begin() + shm[0]);

        // print the nodes mapped to the ith consumer
        cout << "Nodes mapped to consumer " << i << " are: ";
        for (int j = 0; j < shm[0]; j++){
            cout << shm[j+1] << " ";
        }
        cout << endl;

        // detach from the shared memory
        if (shmdt(shm) == -1){
            perror("Error while detaching from shared memory.\n");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}


int main(){

    time_t curr_date;
    time(&curr_date);
    srand(time(0));

    // create shared memory for storing the graph as a set of edges
    key_t key1 = ftok(FILEPATH, 37), key2 = ftok(FILEPATH, 38);

    int shmid1 = shmget(key1, SHM_EDGE_SZ*sizeof(int), IPC_CREAT | 0666);
    if (shmid1 == -1){
        perror("Error while creating shared memory.\n");
        exit(EXIT_FAILURE);
    }

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


    while(1){

        sleep(50);  // sleep for 50 seconds

        int num_nodes = shm1[0], num_edges = shm1[1];
        vector <pair<int, int>> node_degree; // stores the degree of each of the existing nodes
        vector <pair<int, double>> node_prob; // stores the probability of each of the existing nodes being selected

        cout << endl;
        cout << "Number of nodes: " << num_nodes << endl;

        calc_degree(shm1, shm2, num_nodes, num_edges, node_degree, node_prob);  // calculate the degree of each of the existing nodes    

        cout << "Number of edges: " << num_edges << endl;

        // FILE *fp = fopen("producer.txt", "w");
        // if (fp == NULL){
        //     perror("Error while opening file.\n");
        //     exit(EXIT_FAILURE);
        // }

        // for (int i = 0; i < num_nodes; i++){
        //     fprintf(fp, "%d %d %d %lf\n", node_degree[i].first, node_degree[i].second, node_prob[i].first, node_prob[i].second);
        // }
        // fclose(fp);

        vector <int> new_nodes;
        vector <pair<int, int>> new_edges;

        // sample a random number from [10, 30] uniformly and add that many new nodes to the graph
        int num_new_nodes =  10 + rand() % 21;
        cout << "Number of new nodes: " << num_new_nodes << endl;
        int num_new_edges = 0;

        for (int i = 0; i < num_new_nodes; i++){
            new_nodes.push_back(num_nodes + i);

            int k = 1 + rand() % 20;  // sample a random number in [1, 20] uniformly => k is the number of edges to be added for this new node
            num_new_edges += k;

            set <int> node_idx; // stores the indices of the existing nodes selected for the new node

            // sample k nodes from existing nodes with probability proportional to their degree
            for (int j = 0; j < k; j++){
                double rand_num = (double) rand() / RAND_MAX;

                int idx = lower_bound(node_prob.begin(), node_prob.end(), make_pair(0, rand_num), [](pair<int, double> a, pair<int, double> b){
                    return a.second < b.second;
                }) - node_prob.begin();

                // if it is already present, then sample another existing node until a new node is selected (to prevent same edge from being added twice)
                while (node_idx.find(idx) != node_idx.end()){
                    rand_num = (double) rand() / RAND_MAX;
                    idx = lower_bound(node_prob.begin(), node_prob.end(), make_pair(0, rand_num), [](pair<int, double> a, pair<int, double> b){
                        return a.second < b.second;
                    }) - node_prob.begin();
                }

                node_idx.insert(idx); // add the index to the set if it is not already present

                new_edges.push_back(make_pair(num_nodes + i, node_degree[idx].first));
            }     
        }

        cout << "Number of new edges: " << num_new_edges << endl;

        // add the new edges to the shared memory
        for (int i = 0; i < new_edges.size(); i++){
            shm1[2 + num_edges + i] = new_edges[i].first;
            shm2[2 + num_edges + i] = new_edges[i].second;
        }

        // add the edges to the shared memory
        num_nodes += num_new_nodes;
        num_edges += new_edges.size();

        shm1[0] = num_nodes; shm2[0] = num_nodes;
        shm1[1] = num_edges; shm2[1] = num_edges;

        cout << "Number of nodes: " << shm1[0] << endl;
        cout << "Number of edges: " << shm1[1] << endl;

        // map the new nodes equally among the 10 consumer processes using 10 different shared memory segments
        map_nodes_to_cons(new_nodes);

        // FILE *fp1 = fopen("output2.txt", "w");
        // if (fp1 == NULL){
        //     perror("Error while opening file.\n");
        //     exit(EXIT_FAILURE);
        // }

        // for (int i = 0; i < num_edges; i++){
        //     fprintf(fp1, "%d %d\n", shm1[2 + i], shm2[2 + i]);
        // }
        // fclose(fp1);

        cout << endl << endl;
    }

    // detach from the shared memory
    shmdt(shm1); shmdt(shm2);

    return 0;
}


