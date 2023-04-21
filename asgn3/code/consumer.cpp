#include<iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <set>
#include <vector>
#include <algorithm>
#include <time.h>
#include <unistd.h>
#include <string>
#include <queue>
#include <limits.h>
#include <string.h>


#define FILEPATH "facebook_combined.txt"
#define SHM_EDGE_SZ 150000
#define SHM_NODE_MAP_SZ 100
#define MAX_NODES 6000
#define consumer_sleep_time 30

using namespace std;

// function to map the initial set of nodes to the ith consumer 
int map_initial_nodes(int *shm1, int *shm2, int num_nodes, int num_edges, int consumer_id, vector <int> &mapped_nodes){

    int num_nodes_per_consumer = num_nodes/10;
    int rem = num_nodes%10;

    if (consumer_id < rem){      // if the consumer id is less than the remainder, then it gets one more node
        num_nodes_per_consumer++;

        for (int i = 0; i < num_nodes_per_consumer; i++)          // map the nodes to the consumer
            mapped_nodes.push_back(consumer_id*(num_nodes_per_consumer) + i);
    }

    else{      // if the consumer id is greater than the remainder, then it gets the same number of nodes as the other consumers
        
        // map the nodes to the consumer
        for (int i = 0; i < num_nodes_per_consumer; i++){
            mapped_nodes.push_back(rem*(num_nodes_per_consumer + 1) + (consumer_id - rem)*num_nodes_per_consumer + i);
        }
    }

    string filename = "map/consumer_" + to_string(consumer_id) + ".txt";
    FILE *fp = fopen(filename.c_str(), "a");

    // write first the number of nodes and then the nodes themselves
    fprintf(fp, "Number of nodes mapped to consumer %d: %d\n", consumer_id, num_nodes_per_consumer);
    for (int i = 0; i < num_nodes_per_consumer; i++){
        fprintf(fp, "%d\n", mapped_nodes[i]);
    }
    fclose(fp);

    return 0;
}


int dijkstra_algo(int num_nodes, int source, vector<vector<pair<int, int>>> &adj_list, int consumer_id, FILE *fp_res){

    priority_queue <pair<int, int>, vector<pair<int, int>>, greater <pair<int, int>>> pq;  // min heap to store the nodes and their distances from the source 

    // vectors to store the distances and the parent of each node
    vector<int> dist(num_nodes, INT_MAX);
    vector<int> parent(num_nodes, -1);

    // initialize the distance of the source node to 0
    dist[source] = 0;
    pq.push(make_pair(0, source));

    while (!pq.empty()){
        int u = pq.top().second;    // get the node with the minimum distance from the source
        pq.pop();

        // iterate over the adjacency list of the node
        for (int i = 0; i < adj_list[u].size(); i++){
            int v = adj_list[u][i].first;   // get the node
            int weight = adj_list[u][i].second;  // get the weight of the edge

            // if the distance of the node is greater than the distance of the source node + the weight of the edge, then update the distance
            if (dist[v] > dist[u] + weight){
                dist[v] = dist[u] + weight;
                pq.push(make_pair(dist[v], v));
                parent[v] = u;
            }
        }
    }

    fprintf(fp_res, "Shortest path from node %d to all other nodes:\n\n", source);
    for (int i = 0; i < num_nodes; i++){
        if (dist[i] != INT_MAX){
            fprintf(fp_res, "Node %4d: Dist = %2d, Path = ", i, dist[i]);
            vector<int> path;
            int temp = i;
            while (temp != -1){
                path.push_back(temp);
                temp = parent[temp];
            }
            reverse(path.begin(), path.end());
            for (int j = 0; j < path.size(); j++){
                fprintf(fp_res, "%4d ", path[j]);
            }
            fprintf(fp_res, "\n");
        }
        else{
            fprintf(fp_res, "Node %4d: Dist = INF, Path = Does not exist\n", i);
        }
    }

    return 0;

}

int dijkstra_algo_optim(int num_nodes, int num_nodes_prev, int source, vector<vector<pair<int, int>>> &adj_list, int consumer_id, FILE *fp_res, vector<vector<int>> &dist_vector, int print_flag, vector<vector<vector<int>>> &path_vector){

    priority_queue <pair<int, int>, vector<pair<int, int>>, greater <pair<int, int>>> pq; 

    vector<int> dist(num_nodes, INT_MAX);
    vector<int> parent(num_nodes, -1);

    // initialize the distance of the source node to 0
    dist[source] = 0;
    pq.push(make_pair(0, source));

    while (!pq.empty()){
        int u = pq.top().second;    // get the node with the minimum distance from the source
        pq.pop();

        // iterate over the adjacency list of the node
        for (int i = 0; i < adj_list[u].size(); i++){
            int v = adj_list[u][i].first;   // get the node
            int weight = adj_list[u][i].second;  // get the weight of the edge

            // if the distance of the node is greater than the distance of the source node + the weight of the edge, then update the distance
            if (dist[v] > dist[u] + weight){
                dist[v] = dist[u] + weight;
                pq.push(make_pair(dist[v], v));
                parent[v] = u;
            }
        }
    }

    if (print_flag == 1)
        fprintf(fp_res, "Shortest path from node %d to all other nodes:\n\n", source);

    // vector<vector<int>> paths(num_nodes); // vector to store the paths from the source to each node of the graph

    for (int i = 0; i < num_nodes; i++){
        // update the distance vector
        dist_vector[source][i] = dist[i];
        dist_vector[i][source] = dist[i];

        if (dist[i] != INT_MAX){
            if (print_flag == 1)
                fprintf(fp_res, "Node %4d: Dist = %2d, Path = ", i, dist[i]);
       
            int temp = i;
            while (temp != -1){
                path_vector[source - num_nodes_prev][i].push_back(temp);
                temp = parent[temp];
            }
            reverse(path_vector[source - num_nodes_prev][i].begin(), path_vector[source - num_nodes_prev][i].end());
            if (print_flag == 1){
                for (int j = 0; j < path_vector[source - num_nodes_prev][i].size(); j++){
                    fprintf(fp_res, "%4d ", path_vector[source - num_nodes_prev][i][j]);
                }
                fprintf(fp_res, "\n");
            }
        }
        else{
            if (print_flag == 1)
                fprintf(fp_res, "Node %4d: Dist = INF, Path = Does not exist\n", i);
        }
    }

    return 0;
}


// function to compute shortest path using Dijkstra's algorithm
int compute_shortest_path(int num_nodes, int consumer_id, vector<int> &mapped_nodes, vector<vector<pair<int, int>>> &adj_list){

    string filename = "res/consumer_" + to_string(consumer_id) + ".txt";
    FILE *fp_res = fopen(filename.c_str(), "a");

    fprintf(fp_res, "\n\nSHORTEST PATH COMPUTATION CONSIDERING ALL NODES MAPPES TO CONSUMER %d AS SOURCE NODES\n\n", consumer_id);
    cout << "Consumer " << consumer_id << " computing shortest paths..." << endl;

    // multi_src_dijkstra(num_nodes, mapped_nodes, adj_list, consumer_id, fp_res); 

    // for each node mapped to the consumer, compute the shortest path to all other nodes
    for (int j = 0; j < mapped_nodes.size(); j++){

        fprintf(fp_res, "\n\n");
        dijkstra_algo(num_nodes, mapped_nodes[j], adj_list, consumer_id, fp_res);
    }
    fclose(fp_res);
    cout << "Finished computing shortest paths for consumer " << consumer_id << endl << endl;

    return 0;
}

// optimized version of the function to compute shortest path using Dijkstra's algorithm
int compute_shortest_path_optim(int num_nodes, int num_nodes_prev, int consumer_id, vector<int> &mapped_nodes, int num_new_mapped, vector<vector<pair<int, int>>> &adj_list, vector<vector<int>> &dist_vector, int flag){

    string filename = "res_optim/consumer_" + to_string(consumer_id) + ".txt";
    FILE *fp_res = fopen(filename.c_str(), "a");

    fprintf(fp_res, "\n\nSHORTEST PATH COMPUTATION CONSIDERING ALL NODES MAPPES TO CONSUMER %d AS SOURCE NODES\n\n", consumer_id);
    cout << "Consumer " << consumer_id << " computing shortest paths in optimised way ..." << endl;

    // create a 3-D vector to store the path from each newly added node to all other nodes
    vector<vector<vector<int>>> path_vector(num_nodes - num_nodes_prev, vector<vector<int>>(num_nodes));

    if (flag == 0){
        for (int i = 0; i < num_new_mapped; i++){
            fprintf(fp_res, "\n\n");
            dijkstra_algo_optim(num_nodes, 0, mapped_nodes[i], adj_list, consumer_id, fp_res, dist_vector, 1, path_vector);
        }
    }

    if (flag == 1){
        for (int i = 0; i < num_nodes - num_nodes_prev; i++){


            if (find(mapped_nodes.begin() + mapped_nodes.size() - num_new_mapped, mapped_nodes.end(), num_nodes_prev + i) != mapped_nodes.end()){
                fprintf(fp_res, "\n\n");
                dijkstra_algo_optim(num_nodes,num_nodes_prev,  num_nodes_prev + i, adj_list, consumer_id, fp_res, dist_vector, 1, path_vector);
            }
            else {
                dijkstra_algo_optim(num_nodes, num_nodes_prev, num_nodes_prev + i, adj_list, consumer_id, fp_res, dist_vector, 0, path_vector);
            }

        }

        cout << "Updating distances of previous nodes in mapped_set of consumer " << consumer_id << " to all previous other nodes" << endl<<endl;
        // update the distances of the previous nodes in mapped_set of consumer to all previous other nodes    
        for (int i = 0; i < mapped_nodes.size() - num_new_mapped; i++){
            fprintf(fp_res, "\n\n");
            fprintf(fp_res, "Shortest path from node %d to all other nodes:\n\n", mapped_nodes[i]);
        
            for (int j = 0; j < num_nodes_prev; j++){

                if (i != j){
                    int min_idx = INT_MAX;
                    for(int k = 0; k < num_nodes - num_nodes_prev; k++){
                        
                        if (dist_vector[mapped_nodes[i]][j] > dist_vector[mapped_nodes[i]][num_nodes_prev + k] + dist_vector[num_nodes_prev + k][j]){
                            dist_vector[mapped_nodes[i]][j] = dist_vector[mapped_nodes[i]][num_nodes_prev + k] + dist_vector[num_nodes_prev + k][j];
                            min_idx = k;
                        }
                    }

                    if (min_idx != INT_MAX){

                        // print the updated path
                        fprintf(fp_res, "Node %4d: Dist = %2d, Path = ", j, dist_vector[mapped_nodes[i]][j]);

                        for (int l = path_vector[min_idx][mapped_nodes[i]].size() - 1; l >= 0; l--){
                            fprintf(fp_res, "%d ", path_vector[min_idx][mapped_nodes[i]][l]);
                        }

                        for (int l = 1; l < path_vector[min_idx][j].size(); l++){
                            fprintf(fp_res, "%d ", path_vector[min_idx][j][l]);
                        }

                        fprintf(fp_res, "\n");
                    }
                }
            }
        }
    }

    fclose(fp_res);
    cout << "Finished computing shortest paths for consumer " << consumer_id << endl << endl;

    // free the memory allocated to the path_vector
    for (int i = 0; i < num_nodes - num_nodes_prev; i++){
        for (int j = 0; j < num_nodes; j++){
            path_vector[i][j].clear();
        }
        path_vector[i].clear();
    }
    path_vector.clear();

    return 0;
}


int main(int argc, char *argv[]){
    
    int optim_flag = 0;     // flag to check if the user wants to run the optimized version of the program
    int consumer_id = atoi(argv[1]);      // get the consumer id to know which nodes to map
    if (argc == 3){
        if (strcmp(argv[2], "-o") == 0){
            optim_flag = 1;
        }
    }

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

    sleep(consumer_sleep_time); // sleep for 30 seconds to allow the producer to finish writing to the shared memory

    int num_nodes_prev = shm1[0], num_edges_prev = shm1[1];     // get the number of nodes and edges
    vector <int> mapped_nodes;  // vector to store the nodes mapped to the ith consumer process

    map_initial_nodes(shm1, shm2, num_nodes_prev, num_edges_prev, consumer_id, mapped_nodes);     // map the initial set of nodes to the ith consumer
    cout << "Consumer " << consumer_id << " has mapped " << mapped_nodes.size() << " nodes to itself." << endl;
    int num_mapped_nodes = mapped_nodes.size();  // number of new nodes mapped to the consumer

    // create an adjacency list of all nodes in the graph
    vector<vector<pair<int, int>>> adj_list(MAX_NODES);

    for (int i = 0; i < num_edges_prev; i++){
        adj_list[shm1[i + 2]].push_back(make_pair(shm2[i + 2], 1));  // add the edge to the adjacency list of the first node with weight 1
        adj_list[shm2[i + 2]].push_back(make_pair(shm1[i + 2], 1));  // add the edge to the adjacency list of the second node with weight 1
    }

    cout << "Consumer " << consumer_id << ": Adjacency list created." << endl;

    vector<vector<int>> dist_vector(MAX_NODES, vector<int>(MAX_NODES, INT_MAX));    // vector to store the distance of each node from all other nodes

    // function call to compute the initial shortest path (from mapped nodes of consumer i to all other nodes)
    if (optim_flag == 0){
        compute_shortest_path(num_nodes_prev, consumer_id, mapped_nodes, adj_list);
    }
    else if (optim_flag == 1){
        compute_shortest_path_optim(num_nodes_prev, 0, consumer_id, mapped_nodes, num_mapped_nodes ,adj_list, dist_vector, 0);
    }

    // ######## START OF WHILE LOOP #########

    sleep(consumer_sleep_time);

    key_t node_map_key = ftok(FILEPATH, 40 + consumer_id);
    int node_map_shmid = shmget(node_map_key, SHM_NODE_MAP_SZ*sizeof(int), IPC_CREAT | 0666);

    if (node_map_shmid == -1){
        perror("Error while creating shared memory.\n");
        exit(EXIT_FAILURE);
    }
    int *node_map_shm = (int*) shmat(node_map_shmid, NULL, 0);      // attach to the shared memory

    // while loop to keep the consumer process running
    while(1){
    
        cout << "Consumer " << consumer_id << " is awake." << endl;
        // get the updated number of nodes and edges
        int num_nodes_new = shm1[0], num_edges_new = shm1[1];
        // cout << "Consumer:Number of edges: " << num_edges_new << endl;

        if (num_nodes_new == num_nodes_prev && num_edges_new == num_edges_prev){  // if no change made, then sleep for 30 seconds and wake up again
            cout << "No change made to the graph. Consumer " << consumer_id << " is going back to sleep." << endl;
            sleep(consumer_sleep_time);
            continue;
        }

        // get the updated set of nodes mapped to the ith consumer from the shared memory and append it to the mapped_nodes vector
        int num_new_mapped_nodes = node_map_shm[0];
        vector<int> new_mapped_nodes;
        for (int i = 1; i <= num_new_mapped_nodes; i++){
            mapped_nodes.push_back(node_map_shm[i]);
            new_mapped_nodes.push_back(node_map_shm[i]);
        }

        // write the updated set of nodes mapped to the ith consumer to the output file
        string filename = "map/consumer_" + to_string(consumer_id) + ".txt";
        FILE *fp = fopen(filename.c_str(), "a");
        fprintf(fp, "Number of new nodes mapped to consumer %d: %d\n", consumer_id, num_new_mapped_nodes);
        fprintf(fp, "New nodes mapped to consumer %d: \n", consumer_id);
        for (int i = 1; i <= num_new_mapped_nodes; i++){
            fprintf(fp, "%d\n", node_map_shm[i]);
        }

        fclose(fp);

        // update the adjacency list of the graph
        for (int i = num_edges_prev; i < num_edges_new; i++){
            adj_list[shm1[i + 2]].push_back(make_pair(shm2[i + 2], 1));  
            adj_list[shm2[i + 2]].push_back(make_pair(shm1[i + 2], 1));  
        }

        cout << "Consumer " << consumer_id << ": Adjacency list updated." << endl;

        // function call to compute the shortest path (from mapped nodes of consumer i to all other nodes)
        if (optim_flag == 0){
            compute_shortest_path(num_nodes_new, consumer_id, mapped_nodes, adj_list);
        }
        else if (optim_flag == 1){
            compute_shortest_path_optim(num_nodes_new, num_nodes_prev, consumer_id, mapped_nodes, num_new_mapped_nodes, adj_list, dist_vector, 1);
        }
        num_nodes_prev = num_nodes_new;
        num_edges_prev = num_edges_new;

        sleep(consumer_sleep_time);
    }
    return 0;
}