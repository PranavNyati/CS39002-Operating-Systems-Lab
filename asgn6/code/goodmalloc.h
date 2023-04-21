#ifndef GOOD_MALLOC_H
#define GOOD_MALLOC_H

#include <iostream>
#include<stdlib.h>
#include<string>
#include<vector>
#include<map>
#include<queue>
#include<stack>
#include<math.h>
#include<utility>
#include<set>
#include<cstring>
#include<functional>

using namespace std;

#define NUM_ELEM_PER_PAGE 20

typedef struct {

    int *mem;
    unsigned long capacity;

    //min heap of <int> to store the free pages
    priority_queue<int , vector<int>, greater<int>> free_list;
    unsigned long num_free_pages;

    void init(size_t size);
    void printMemPage(unsigned long page_num);
    void printFreeList();

} mem_Block;

typedef struct {

    unsigned long next_entry;
    unsigned long size;
    unsigned long capacity;    
    vector< unsigned long> *list_pages;

} Sym_Table_Entry;

typedef struct {

    size_t first_free_entry;
    size_t num_free_entries;
    Sym_Table_Entry *entries;

    void init();
    int addEntry(int scope, string list_name, vector<unsigned long> page_mapping, unsigned long capacity);
    void deleteEntry(int scope, string list_name);
    void printSymTable();
    void printEntry(int scope, string list_name);

}Sym_Table;


int createMem(size_t size);

int createList(size_t size, string list_name);

void assignVal(string list_name, unsigned long offset, int *value);

void getVal(string list_name, unsigned long offset, int *value);

void freeList(string list_name);

void printList(string list_name);

unsigned long compute_mem_address(unsigned long entry_num, unsigned long offset);

void initScope();

void endScope();


#endif