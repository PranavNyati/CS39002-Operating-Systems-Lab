#include <iostream>
#include "goodmalloc.h"
#include <time.h>   

#define MAX_VAL 100000

using namespace std;


int getRandomNumber(int upper = MAX_VAL,int lower = 1){
    return ((rand()%(upper-lower))+ lower);
}

void merge_sort(string list_name, unsigned int start, unsigned int n){

    initScope();

    if (n <= 1){

        endScope();
        return;
    }

    unsigned int mid = n/2;

    merge_sort(list_name, start, mid);
    merge_sort(list_name, start + mid, n - mid);

    createList(n, "temp");

    unsigned int i = start;
    unsigned int j = start + mid;
    unsigned int k = 0;

    while (i < start + mid && j < start + n){
        int val1, val2;
        getVal(list_name, i, &val1);
        getVal(list_name, j, &val2);
        if (val1 < val2){
            assignVal("temp", k, &val1);
            i++;
        }
        else{
            assignVal("temp", k, &val2);
            j++;
        }
        k++;
    }

    while (i < start + mid){
        int val;
        getVal(list_name, i, &val);
        assignVal("temp", k, &val);
        i++;
        k++;
    }

    while (j < start + n){
        int val;
        getVal(list_name, j, &val);
        assignVal("temp", k, &val);
        j++;
        k++;
    }

    for (unsigned int i = start; i < start + n; i++){
        int val;
        getVal("temp", i - start, &val);
        assignVal(list_name, i, &val);
    }

    freeList("temp");

    endScope();

    return;
}


int main(){

    srand(time(NULL));

    size_t mem_size = 250 * 1024 * 1024; // create a 250 MB memory
    createMem(mem_size);

    initScope();

    size_t list_size = 50000;

    cout << endl;
    cout << "Creating an array of " << list_size << " elements..." << endl;

    // create an array of 50,000 elements
    createList(list_size, "list1");

    // min heap to store the elements of the array
    priority_queue<int, vector<int>, greater<int>> pq;

    cout << "Assigning random values to the array..." << endl;
    cout << endl;

    // assign random values to array elements
    for (int i = 0; i < list_size; i++){
        int val = getRandomNumber();
        assignVal("list1", i, &val);
        pq.push(val);
    }

    // printList("list1");
    FILE *fp_0 = fopen("unsort.txt", "w");
    // cout << "LIST BEFORE SORTING: " << endl;
    // cout << "-------------------" << endl;
    for (int i = 0; i < list_size; i++){
        int val;
        getVal("list1", i, &val);
        fprintf(fp_0, "%d\n", val);
    }
    fclose(fp_0);
    // cout << endl;
    // cout << "-------------------" << endl;

    cout << "Sorting the array..." << endl;

    // time the mergesort function
    clock_t start = clock();
    merge_sort("list1", 0, list_size);
    clock_t end = clock();

    cout << "Sorting complete!" << endl;

    // time elapsed in milliseconds
    double elapsed = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;


    // print the sorted array
    // printList("list1");
    // cout << "LIST AFTER SORTING: " << endl;

    for (int i = 0; i < list_size; i++){
        int val;
        getVal("list1", i, &val);
        if (val != pq.top()){
            cout << "ERROR: " << val << " " << pq.top() << endl;
            cout << "i: " << i << endl;
            cout << "NOT SORTED!" << endl;
            return 0;
        }
        pq.pop();
    }
    cout << endl;
    cout << "SORTED!" << endl;

    FILE *fp = fopen("sort.txt", "w");

    // cout << "-------------------" << endl;
    for (int i = 0; i < list_size; i++){
        int val;
        getVal("list1", i, &val);
        fprintf(fp, "%d\n", val);
    }
    fclose(fp);
    // cout << endl;
    // cout << "-------------------" << endl;
    

    cout << endl;

    // FILE *fp_1 = fopen("time_without_free.txt", "a");
    printf("Time taken to sort %ld element array: %f ms\n", list_size, elapsed);

    // fprintf(fp_1, "%f\n", elapsed);
    // fclose(fp_1);
    cout << "Freeing the list..." << endl;
    freeList("list1");
    cout << "Exiting..."<< endl;

    endScope();

    return 0;
}
