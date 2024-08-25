#include<iostream>
#include<pthread.h>
#include<signal.h>
#include<unistd.h>
#include<time.h>
#include<math.h>
#include <bits/stdc++.h>

#define GRAPH_FILE "musae_git_edges.csv"
#define NUM_USER_SIM_THREADS 1
#define NUM_PUSH_UPDATE_THREADS 25
#define NUM_READ_POST_THREADS 10
#define NUM_NODES 37700
#define NUM_EDGES 289003
#define MAX_ITERS 5

using namespace std;

//struct for an action (post, like, comment)
typedef struct _Action{
    int user_id;  // id of the user(node) who performed the action
    int action_id; // id of the action (post, like, comment) performed by the user
    int action_type; // 0: post, 1: like, 2: comment
    time_t timestamp; // time at which the action was performed
}Action;

struct myComp {
    constexpr bool operator()(
        pair<int, Action> const& a,
        pair<int, Action> const& b)
        const noexcept
    {
        return a.first < b.first;
    }
};

// struct for node of a graph
typedef struct _Node{
    int node_id;
    int degree;
    int action_ctr[3];
    int order_by;  // flag to display the Feed in order of popularity or recency (0: priority, 1: chronological)

    vector <pair <int, int> > neighbors; // vector to store the neighbors of the node along with their priority

    queue <Action> wall_queue; // queue to store the actions performed by the user
    queue <Action> feed_queue; // queue to store the actions performed by the user's friends(neighbors)
    priority_queue <pair<int,Action>, vector<pair<int,Action>>, myComp> priority_feed_queue;       // a max heap to store the actions performed by the user's friends(neighbors) in order of their priority

    pthread_mutex_t feed_mutex; // mutex to lock the feed_queue of the user
}Node;


// global variables and data structures
vector <Node> graph;  
queue <Action> new_action_q; // queue to store the new actions performed by the users
queue <int> feed_update_q;  // queue to store the user_ids of the nodes whose feed got updated due to pushUpdate thread pushing new actions to their feed_queue

pthread_mutex_t action_q_mutex;  // mutex to lock the new_action_q
pthread_mutex_t logfile_mutex;   // mutex to lock the logfile
pthread_mutex_t feed_update_q_mutex;   // mutex to lock the common queue shared between pushUpdate and readPost threads

pthread_cond_t action_q_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t feed_update_q_cond = PTHREAD_COND_INITIALIZER;

map <int, string > action_map = {{0, "POST"}, {1, "LIKE"}, {2, "COMMENT"}};

int thread_to_join = 0;
FILE *fp;

int getRandomNumber(int upper = NUM_NODES,int lower = 0){
    return ((rand()%(upper-lower))+ lower);
}

// initialize a Node
void initNode(Node *node, int node_id){
    node->node_id = node_id;
    node->degree = 0;
    node->action_ctr[0] = 0;
    node->action_ctr[1] = 0;
    node->action_ctr[2] = 0;
    node->order_by = getRandomNumber(2); // 1: chronological, 0: priority
    pthread_mutex_init(&node->feed_mutex, NULL);
}

// initialize the graph
void initGraph(){

    for(int i=0; i<NUM_NODES; i++){
        Node node;
        initNode(&node, i);
        graph.push_back(node);
    }
}

int calcPriorityUtil(Node *n1,Node *n2){
    int common = 0;
    for(int i = 0;i<n1->degree;i++){
        for(int j = 0;j<n2->degree;j++){
            if (n1->neighbors[i].first == n2->neighbors[j].first){
                common++;
            }
        }
    }
    return common;
}

// function to calculate priority between pair of nodes (= number of common neighbors)
void calcPriority(){

    for(int i = 0;i<NUM_NODES;i++){
    
        for (int j = 0; j < graph[i].neighbors.size(); j++){
            int neighbor_id = graph[i].neighbors[j].first;
            int priority = calcPriorityUtil(&graph[i], &graph[neighbor_id]);
            graph[i].neighbors[j].second = priority;
        }
    }
}


// function to read the graph from the file, and calculate the priority of each node with respect to its neighbors
void readGraph(){
    FILE *fp1 = fopen(GRAPH_FILE, "r");
    if(fp1 == NULL){
        cout << "Error opening the file" << endl;
        exit(1);
    }
    char line[100];
    int ctr = 0;
    while(fgets(line, 100, fp1) != NULL){
        if(ctr == 0){
            ctr++;
            continue;
        }
        ctr++;
        int node1, node2;
        sscanf(line, "%d,%d", &node1, &node2);
        graph[node1].neighbors.push_back(make_pair(node2, 0));
        graph[node2].neighbors.push_back(make_pair(node1, 0));
        graph[node1].degree++;   graph[node2].degree++;
    }
    cout << "Graph read successfully" << endl;

    cout << "Computing priorities among nodes..." << endl;
    // calculate the priority of each node with respect to each of its neighbors
    calcPriority(); 

    // sort the neighbors of each node in decreasing order of their priority
    for(int i = 0;i<NUM_NODES;i++){
        sort(graph[i].neighbors.begin(), graph[i].neighbors.end(), [](const pair <int, int> &a, const pair <int, int> &b){
            return a.second > b.second;
        });
    }

    cout << "Priorities among nodes computed!" << endl;
    fclose(fp1);
}

// userSimulator thread function
void *userSimulator(void *arg){
    srand(time(NULL));
    sleep(6);

    int itr = 0;

    while (1){
        pthread_mutex_lock(&logfile_mutex);
        fprintf(fp, "[ US ] => ITERATION: %d\n\n", itr);
        fflush(fp);
        cout << "[ US ] => ITERATION: " << itr << endl;
        pthread_mutex_unlock(&logfile_mutex);

        // select 100 random nodes, and for each node, generate 10*(1 + log(degree)) actions
        set <int> node_ids;
        int total_actions = 0;

        for (int i = 0; i < 100; i++){
            int node_id = getRandomNumber();

            while (node_ids.find(node_id) != node_ids.end())   // if node_id is already present in the set, generate a new node_id
                node_id = getRandomNumber();

            node_ids.insert(node_id);

            int num_actions = 10 * (1 + log2(graph[node_id].degree));
            total_actions += num_actions;

            pthread_mutex_lock(&logfile_mutex);
            fprintf(fp, "[ US ] => i: %d, Node_id: %d, Num_actions: %d\n", i, node_id, num_actions);
            fflush(fp);
            cout << "[ US ] => i: " << i << ", Node_id: " << node_id << ", Num_actions: " << num_actions << endl;
            pthread_mutex_unlock(&logfile_mutex);

            vector<Action> actions(num_actions);
            for (int j = 0; j < num_actions; j++){
                actions[j].user_id = node_id;
                actions[j].action_type = getRandomNumber(3, 0);
                graph[node_id].action_ctr[actions[j].action_type] += 1;
                actions[j].action_id = graph[node_id].action_ctr[actions[j].action_type];
                actions[j].timestamp = time(NULL);

                // push the action into the Wall queue of the user
                graph[node_id].wall_queue.push(actions[j]);

                pthread_mutex_lock(&logfile_mutex);
                fprintf(fp, "[ US ] Generated an action with Node_id %d: Action_num %d: Type: %s, ID: %d\n", node_id, j, action_map[actions[j].action_type].c_str(), actions[j].action_id);
                fflush(fp);
                pthread_mutex_unlock(&logfile_mutex);
   
            }

            // push all the actions of a node into the new_action_queue
            for (int j = 0; j < num_actions; j++){
                pthread_mutex_lock(&action_q_mutex);
                new_action_q.push(actions[j]);
                pthread_cond_broadcast(&action_q_cond);
                pthread_mutex_unlock(&action_q_mutex);
            }
            actions.clear();
        }

        pthread_mutex_lock(&logfile_mutex);
        fprintf(fp, "[ US ] Total actions generated considering all 100 nodes for itrn %d: %d\n", itr, total_actions);
        cout << "[ US ] Total actions generated considering all 100 nodes for itrn " << itr << ": " << total_actions << endl;
        pthread_mutex_unlock(&logfile_mutex);

        sleep(120); // sleep for 2 minutes

        pthread_mutex_lock(&logfile_mutex);
        cout << "[ US ] Iteration " << itr << " completed!" << endl;
        pthread_mutex_unlock(&logfile_mutex);
        
        itr++;

        if (itr == MAX_ITERS)
        {
            pthread_mutex_lock(&logfile_mutex);
            cout << "Exiting userSimulator thread" << endl;
            pthread_mutex_unlock(&logfile_mutex);

            // broadcast the condition variable to all the threads waiting on it that the userSimulator thread has exited
            pthread_mutex_lock(&action_q_mutex);
            thread_to_join++;
            pthread_cond_broadcast(&action_q_cond);
            pthread_mutex_unlock(&action_q_mutex);

            break;
        }
    }
    pthread_exit(0); 
}

// // pushUpdate thread function
void *pushUpdate(void *arg){

    int thread_id = *((int *)arg);

    int flag = 0;
    // invoke the pushUpdate function whenever the size of the new_action_queue increases using condition variables
    while(1){
        pthread_mutex_lock(&action_q_mutex);

        cout << "[ PU "<< thread_id << " ] " << "Push Thread " << thread_id << " waiting!" << endl;

        while (new_action_q.empty()){
            pthread_cond_wait(&action_q_cond, &action_q_mutex);

            if (thread_to_join >= NUM_USER_SIM_THREADS){
                flag = 1;
                break;
            }
        }
        
        if (flag == 1){
            pthread_mutex_unlock(&action_q_mutex);
            break;
        }

        Action action = new_action_q.front();
        new_action_q.pop();

        pthread_mutex_lock(&logfile_mutex);
        fprintf(fp, "[ PU %d ] => Th %d pops action %d of type %s of user %d!\n", thread_id, thread_id, action.action_id, action_map[action.action_type].c_str(), action.user_id);
        fflush(fp);
        cout << "[ PU " << thread_id << " ] => Th " << thread_id << " pops action " << action.action_id << " of type " << action_map[action.action_type] << " of user " << action.user_id << "!" << endl;
        cout << "Action queue size: " << new_action_q.size() << endl;
        pthread_mutex_unlock(&logfile_mutex);

        pthread_mutex_unlock(&action_q_mutex);

        // after popping an action, push the action to the Feed queue of all the neighbors of the user(node) who performed the action
        for (int i = 0; i < graph[action.user_id].degree; i++){
            int neighbor_id = graph[action.user_id].neighbors[i].first;

            // push the action into the central read_update_q common to all the pushUpdate and readPost threads
            pthread_mutex_lock(&feed_update_q_mutex);
            feed_update_q.push(neighbor_id);
            pthread_cond_broadcast(&feed_update_q_cond);
            pthread_mutex_unlock(&feed_update_q_mutex);

            // push the action into the Feed queue of the neighbor
            pthread_mutex_lock(&graph[neighbor_id].feed_mutex);
            if (graph[neighbor_id].order_by == 1){
                graph[neighbor_id].feed_queue.push(action);
            }
            else if (graph[neighbor_id].order_by == 0){
                graph[neighbor_id].priority_feed_queue.push(make_pair(graph[action.user_id].neighbors[i].second, action));
            }
            pthread_mutex_unlock(&graph[neighbor_id].feed_mutex);
        }

        // write that action pushed to the Feed queue of all the neighbors to the sns.log file
        pthread_mutex_lock(&logfile_mutex);
        fprintf(fp, "[ PU %d ] => Th %d pushes action %d of type %s of user %d to all its nbrs!\n", thread_id, thread_id, action.action_id, action_map[action.action_type].c_str(), action.user_id);
        fflush(fp);
        cout << "[ PU " << thread_id << " ] => Th " << thread_id << " pushes action " << action.action_id << " of type " << action_map[action.action_type] << " of user " << action.user_id << " to all its nbrs!" << endl;
        pthread_mutex_unlock(&logfile_mutex);

    }

    sleep(4);
    
    // signal the readPost threads that the pushUpdate thread has terminated
    pthread_mutex_lock(&feed_update_q_mutex);
    thread_to_join++;
    pthread_cond_broadcast(&feed_update_q_cond);
    pthread_mutex_unlock(&feed_update_q_mutex);

    pthread_mutex_lock(&logfile_mutex);
    cout << "Terminating PushUpdate thread " << thread_id << endl;
    pthread_mutex_unlock(&logfile_mutex);

    pthread_exit(0);
}


// readPost thread function
void *readPost(void *arg){
    
    int thread_id = *((int *)arg);
    int flag = 0;

    while(1){
        pthread_mutex_lock(&feed_update_q_mutex);

        cout << "[ RP "<< thread_id << " ] " << "Read Thread " << thread_id << " waiting!" << endl;

        while (feed_update_q.empty()){
            pthread_cond_wait(&feed_update_q_cond, &feed_update_q_mutex);

            if ( (thread_to_join >= NUM_USER_SIM_THREADS + NUM_PUSH_UPDATE_THREADS) && (feed_update_q.empty()) ){
                flag = 1;
                break;
            }
        }

        if (flag == 1){
            pthread_mutex_unlock(&feed_update_q_mutex);
            break;
        }

        pthread_mutex_lock(&logfile_mutex);
        cout << "Feed queue size: " << feed_update_q.size() << endl;
        pthread_mutex_unlock(&logfile_mutex);

        int user_id = feed_update_q.front();
        feed_update_q.pop();

        pthread_mutex_unlock(&feed_update_q_mutex);

        if (graph[user_id].order_by == 1){  // order by timestamp (chronological order)

            // read the Feed queue of the user_id
            pthread_mutex_lock(&graph[user_id].feed_mutex);
            while (!graph[user_id].feed_queue.empty()){
                Action action = graph[user_id].feed_queue.front();
                graph[user_id].feed_queue.pop();

                // convert timestamp to string
                time_t t = action.timestamp;
                struct tm *tm = localtime(&t);
                char date[20];
                strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", tm);

                pthread_mutex_lock(&logfile_mutex);
                fprintf(fp, "[ RP %d ] => Th %d reads action %d of type %s posted by user %d (chrono) at %s from neighbor %d!\n", thread_id, thread_id, action.action_id, action_map[action.action_type].c_str(), action.user_id, date, user_id);
                fflush(fp);
                // cout << "[ RP " << thread_id << " ] => Th " << thread_id << " reads action " << action.action_id << " of type " << action_map[action.action_type] << " posted by user " << action.user_id << " (chrono) at " << date << " from neighbor " << user_id << "!" << endl;
                pthread_mutex_unlock(&logfile_mutex);
            }
            pthread_mutex_unlock(&graph[user_id].feed_mutex);
        }

        else if (graph[user_id].order_by == 0){ // order by priority

            // read the Priority Feed queue of the user_id
            pthread_mutex_lock(&graph[user_id].feed_mutex);

            while (!graph[user_id].priority_feed_queue.empty()){
                Action action = graph[user_id].priority_feed_queue.top().second;
                int priority = graph[user_id].priority_feed_queue.top().first;
                graph[user_id].priority_feed_queue.pop();

                // pthread_mutex_unlock(&graph[user_id].feed_mutex);

                // convert timestamp to string
                time_t t = action.timestamp;
                struct tm *tm = localtime(&t);
                char date[20];
                strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", tm);

                pthread_mutex_lock(&logfile_mutex);
                // fprintf(fp, "[ RP %d ] => User_id: %d Priority Feed Queue size: %ld\n", thread_id, user_id,  graph[user_id].priority_feed_queue.size());
                fprintf(fp, "[ RP %d ] => Th %d reads action %d of type %s posted by user %d (with p = %d) at %s from neighbor %d!\n", thread_id, thread_id, action.action_id, action_map[action.action_type].c_str(), action.user_id, priority, date, user_id);
                fflush(fp);
                // cout << "[ RP " << thread_id << " ] => Th " << thread_id << " reads action " << action.action_id << " of type " << action_map[action.action_type] << " posted by user " << action.user_id << " (with p = " << priority << ") at " << date << " from neighbor " << user_id << "!" << endl;
                pthread_mutex_unlock(&logfile_mutex);

                // pthread_mutex_lock(&graph[user_id].feed_mutex);
            }
            pthread_mutex_unlock(&graph[user_id].feed_mutex);
        }
    }

    pthread_mutex_lock(&logfile_mutex);
    cout << "Exiting readPost thread " << thread_id << endl;
    pthread_mutex_unlock(&logfile_mutex);

    pthread_exit(0);
}


int main(){

    initGraph();      // initialize the graph
    readGraph();      // read the graph from the file

    fp  = fopen("sns.log", "a");

    // create 1 userSimulator thread, 25 pushUpdate threads, 10 readPost threads
    pthread_t userSim_thread, pushUpdate_thread[NUM_PUSH_UPDATE_THREADS], readPost_thread[NUM_READ_POST_THREADS];

    pthread_mutex_init(&action_q_mutex, NULL);    
    pthread_mutex_init(&feed_update_q_mutex, NULL);                  // initialize the feed_update_q_mutex
    pthread_mutex_init(&logfile_mutex, NULL);                       // initialize the logfile_mutex

    if (pthread_create(&userSim_thread, NULL, userSimulator, NULL) != 0) { // create the userSimulator thread
        perror("Error creating userSimulator thread!");
        exit(EXIT_FAILURE);
    }

    for(int i=0; i<NUM_PUSH_UPDATE_THREADS; i++){                // create the pushUpdate threads      
        pthread_create(&pushUpdate_thread[i], NULL, pushUpdate, &i);
        fprintf(fp, "[ Main ] => Created pushUpdate thread %d\n", i);
        cout << "[ Main ] => Created pushUpdate thread " << i << endl; 
        sleep(0.2);
    }

    for(int i=0; i<NUM_READ_POST_THREADS; i++){                  // create the readPost threads

        pthread_create(&readPost_thread[i], NULL, readPost, &i);
        sleep(0.1);
        pthread_mutex_lock(&logfile_mutex);
        fprintf(fp, "[ Main ] => Created readPost thread %d\n", i);
        fflush(fp);
        cout << "[ Main ] => Created readPost thread " << i << endl;
        pthread_mutex_unlock(&logfile_mutex);
    }

    pthread_join(userSim_thread, NULL);                             // join the userSimulator thread

    for(int i=0; i<NUM_PUSH_UPDATE_THREADS; i++){                    // join the pushUpdate threads
        pthread_join(pushUpdate_thread[i], NULL);
    }
    
    for(int i=0; i<NUM_READ_POST_THREADS; i++){                     // join the readPost threads
        pthread_join(readPost_thread[i], NULL);
        thread_to_join ++;
    }

    cout << "[ Main ] All readPost threads joined: thread_to_join = " << thread_to_join << endl;

    fclose(fp);
    return 0;
}

