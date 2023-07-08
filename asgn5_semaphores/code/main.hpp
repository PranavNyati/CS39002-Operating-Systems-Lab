#include<iostream>
#include<sys/wait.h>
#include<unistd.h>
#include<pthread.h>
#include<semaphore.h>
#include<queue>
#include<utility>
#include<vector>
#include<set>
// #include<bits/stdc++.h>

#define NUM_ITRS 5

typedef struct _room{
    int room_no;
    int count;
    int curr_occupied;
    int guest_id;
    int clean_time = 0;
}room;

typedef struct _guest{
    int guest_id;
    int priority;
} guest;

extern room *hotel_rooms;
extern guest *guests;

struct myComp{
    constexpr bool operator()(std::pair<room *, int> const &a, std::pair<room *, int> const &b) const noexcept
    { return a.second > b.second; }
};

extern std:: queue <room *> empty_q; // queue of empty rooms
extern std:: priority_queue <std::pair<room *, int>, std::vector<std::pair<room *, int>>, myComp> one_occ_q; // queue in which 1 guest has already stayed and is waiting for another guest
extern std:: queue <room *> one_free_q; // queue in which 1 guest has already stayed and is waiting for another guest
extern std:: queue <room *> two_occ_q; // queue in which 2 guests have already stayed and is waiting to be cleaned

extern sem_t q1_sem, q2_sem, q3_sem, q4_sem;
extern sem_t write_sem;
extern sem_t *room_sem;


extern pthread_mutex_t signal_mutex;
extern pthread_cond_t signal_cond;

extern pthread_mutex_t all_clean_mutex;
extern pthread_cond_t all_clean_cond;

extern pthread_mutex_t signal_guests_mutex;
extern pthread_cond_t signal_guests_cond;

extern pthread_mutex_t temp_two_occ_q_mutex;
extern pthread_cond_t temp_two_occ_q_cond;

// Global variables
extern int num_guest_threads;
extern int num_cleaner_threads;
extern int num_hotel_rooms;
extern int num_cleaned_rooms;
extern int use_count;


void *cleaner_func(void *arg);
void *guest_func(void *arg);