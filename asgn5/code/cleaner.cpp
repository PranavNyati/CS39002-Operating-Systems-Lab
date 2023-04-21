#include "main.hpp"

using namespace std;

void *cleaner_func(void *arg){
    int *id = (int *)arg;
    cout << "Cleaner thread " << *id << " is running" << endl;

    while(1){

        pthread_mutex_lock(&signal_mutex);
        while(use_count < 2*num_hotel_rooms){
            
            sem_wait(&write_sem);
            cout << "[CT]: Cleaner thread " << *id << " is waiting for all rooms to be used 2 times" << endl;
            sem_post(&write_sem);

            pthread_cond_wait(&signal_cond, &signal_mutex);

            pthread_mutex_lock(&all_clean_mutex);
            num_cleaned_rooms = 0;
            pthread_mutex_unlock(&all_clean_mutex);


            sem_wait(&write_sem);
            cout << "[CT]: Cleaner " << *id << " is ready to clean rooms!" << endl;
            sem_post(&write_sem);
        }
        pthread_mutex_unlock(&signal_mutex);



        // pop an element from the two_occ_q
        sem_wait(&q4_sem);
        if (!two_occ_q.empty()){
            room *r = two_occ_q.front();
            two_occ_q.pop();
            // cout << "Two occ queue size: " << two_occ_q.size() << endl;
            sem_post(&q4_sem);


            // reset the room attributes
            r->count = 0;
            r->curr_occupied = 0;
            r->guest_id = -1;
            int clean_time = r->clean_time;
            r->clean_time = 0;

            sem_wait(&write_sem);
            cout << "[CT]: Cleaner " << *id << " will clean room " << r->room_no << " for " << clean_time << " seconds!" << endl;
            sem_post(&write_sem);

            sleep(clean_time);

            sem_wait(&write_sem);
            cout << "[CT]: Cleaner " << *id << " has cleaned room " << r->room_no << "!" << endl;
            sem_post(&write_sem);

            // push the cleaned room into the empty_q
            sem_wait(&q1_sem);
            empty_q.push(r);
            // cout << "Empty queue size: " << empty_q.size() << endl;
            sem_post(&q1_sem);


            pthread_mutex_lock(&all_clean_mutex);
            num_cleaned_rooms++;
            if (num_cleaned_rooms == num_hotel_rooms){
                use_count = 0;
                pthread_cond_broadcast(&all_clean_cond);

                sem_wait(&write_sem);
                cout << "[CT]: Cleaner " << *id << " ALL ROOMS CLEANED!" << endl;
                sem_post(&write_sem);

                pthread_mutex_unlock(&all_clean_mutex);

                sleep(1);

                pthread_mutex_lock(&signal_guests_mutex);
                pthread_cond_broadcast(&signal_guests_cond);  // signal all the waiting guests
                pthread_mutex_unlock(&signal_guests_mutex);
            }
            pthread_mutex_unlock(&all_clean_mutex);

        }

        else {
            sem_post(&q4_sem);

            pthread_mutex_lock(&all_clean_mutex);
            while(num_cleaned_rooms < num_hotel_rooms){
                pthread_cond_wait(&all_clean_cond, &all_clean_mutex);

                // sem_wait(&write_sem);
                // cout << "[CT]: Cleaner " << *id << ": Temporary wait over: All rooms cleaned!" << endl;
                // sem_post(&write_sem);
            }
            pthread_mutex_unlock(&all_clean_mutex);
        }
    }
}
