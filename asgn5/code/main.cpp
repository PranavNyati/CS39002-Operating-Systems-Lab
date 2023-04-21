
#include "main.hpp"

using namespace std;

// global variables
int num_hotel_rooms = 0, num_guest_threads = 0, num_cleaner_threads = 0, num_cleaned_rooms = 0;
int use_count = 0;
room *hotel_rooms;
guest *guests;

queue <room *> empty_q; // queue of empty rooms
priority_queue <pair<room *, int>, vector<pair<room *, int>>, myComp> one_occ_q; // queue in which 1 guest has already stayed and is waiting for another guest
queue <room *> one_free_q; // queue in which 1 guest has already stayed and is waiting for another guest
queue <room *> two_occ_q; // queue in which 1 guest has already stayed and 2nd guest is staying

sem_t q1_sem, q2_sem, q3_sem, q4_sem;
sem_t write_sem;
sem_t *room_sem;

pthread_mutex_t signal_mutex;
pthread_cond_t signal_cond;

pthread_mutex_t all_clean_mutex;
pthread_cond_t all_clean_cond;

pthread_mutex_t temp_two_occ_q_mutex;
pthread_cond_t temp_two_occ_q_cond;

pthread_mutex_t signal_guests_mutex;
pthread_cond_t signal_guests_cond;

void init_hotel_rooms(){
    for (int i = 0; i < num_hotel_rooms; i++){
        hotel_rooms[i].room_no = i;
        hotel_rooms[i].count = 0;
        hotel_rooms[i].curr_occupied = 0;
        hotel_rooms[i].guest_id = -1;
        hotel_rooms[i].clean_time = 0;
    }
    return;
}

void init_semaphores(){
    sem_init(&q1_sem, 0, 1);
    sem_init(&q2_sem, 0, 1);
    sem_init(&q3_sem, 0, 1);
    sem_init(&q4_sem, 0, 1);

    sem_init(&write_sem, 0, 1);

    room_sem = new sem_t[num_hotel_rooms];
    for (int i = 0; i < num_hotel_rooms; i++){
        sem_init(&room_sem[i], 0, 0);
    }

    pthread_mutex_init(&signal_mutex, NULL);
    pthread_cond_init(&signal_cond, NULL);

    pthread_mutex_init(&all_clean_mutex, NULL);
    pthread_cond_init(&all_clean_cond, NULL);

    pthread_mutex_init(&temp_two_occ_q_mutex, NULL);
    pthread_cond_init(&temp_two_occ_q_cond, NULL);

    pthread_mutex_init(&signal_guests_mutex, NULL);
    pthread_cond_init(&signal_guests_cond, NULL);
}

void init_guests(){
    for (int i = 0; i < num_guest_threads; i++){
        guests[i].guest_id = i;
        guests[i].priority = ( rand() % num_guest_threads ) + 1;
    }
    return;
}


int main(){

    time_t t = time(NULL);
    unsigned int seed = 1679366315; 
    srand(seed);

    cout << "Seed: " << seed << endl;

    cout << "Enter the number of Hotel rooms: "<<endl;
    cin >> num_hotel_rooms;
    cout << "Enter number of guest threads: "<<endl;
    cin >> num_guest_threads;
    cout << "Enter number of cleaner threads: "<<endl;
    cin >> num_cleaner_threads;

    // initialize the hotel_rooms struct
    hotel_rooms = new room[num_hotel_rooms];
    init_hotel_rooms();

    // initialize the guests struct
    guests = new guest[num_guest_threads];
    init_guests();

    // initialize the semaphores
    init_semaphores();

    // initialize the queues
    for (int i = 0; i < num_hotel_rooms; i++){
        empty_q.push(&hotel_rooms[i]);
    }

    pthread_t guest_th[num_guest_threads], cleaner_th[num_cleaner_threads];

    for (int i = 0; i < num_guest_threads; i++){
        int *id = new int;
        *id = i;
        pthread_create(&guest_th[i], NULL, guest_func, id);
        sleep(0.25);
    }

    for (int i = 0; i < num_cleaner_threads; i++){
        int *id = new int;
        *id = i;
        pthread_create(&cleaner_th[i], NULL, cleaner_func, id);
        sleep(0.25);
    }


    // join the threads

    for (int i = 0; i < num_guest_threads; i++){
        pthread_join(guest_th[i], NULL);
    }

    for (int i = 0; i < num_cleaner_threads; i++){
        pthread_join(cleaner_th[i], NULL);
    }

    printf("All threads joined\n");
    printf("Exiting main thread\n");

    return 0;
}