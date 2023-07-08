#include "goodmalloc.h"


mem_Block *mem = NULL;
Sym_Table *sym_table = NULL;

unsigned long MAX_PAGES_USED = 0;

int curr_scope = -1;

size_t NUM_SYM_TABLE_ENTRIES = 100000;

unsigned long PAGE_SIZE = NUM_ELEM_PER_PAGE*sizeof(int);

map <pair<int , string >, unsigned long> list_map;

int createMem(size_t size){
    
    if (mem != NULL){
        cout << "Error: Memory already initialized!" << endl;
        return-1;
    }

    // ceil to nearest page size
    size_t mem_size =  (size_t)ceil((double)size/(double)PAGE_SIZE) * PAGE_SIZE;

    // initialize memory block
    mem = (mem_Block*)malloc(sizeof(mem_Block));
    mem->init(mem_size);

    cout << "Memory initialized with size: " << mem_size << " bytes" << endl;
    cout << "Total number of pages: " << mem_size/PAGE_SIZE << endl;

    // initialize Sym Tables
    if (sym_table == NULL){
        sym_table = (Sym_Table*)malloc(sizeof(Sym_Table));
        sym_table->init();

        cout << "Symbol Table initialized with size: " << NUM_SYM_TABLE_ENTRIES << " entries" << endl;
    }
    else{
        cout << "Error: Symbol Table already initialized!" << endl;
        return -1;
    }
    return 0;
}

void mem_Block::init(size_t size){

    mem = (int *)malloc(size);
    // memset(mem, 0, sizeof(mem));
    for (size_t i = 0; i < size/sizeof(int); i++){
        mem[i] = 0;
    }
    
    capacity = size;  // in bytes
    num_free_pages = size/PAGE_SIZE;
    
    // initialize free pages
    for (size_t i = 0; i < num_free_pages; i++){
        free_list.push(i);
    }

    return;
}

void mem_Block::printMemPage(unsigned long page_num){

    if (page_num < 0 || page_num >= capacity/PAGE_SIZE){
        cout << "Error: Trying to access invalid memory address!" << endl;
        return;
    }

    cout << "Page " << page_num << ": ";
    for (size_t i = 0; i < NUM_ELEM_PER_PAGE; i++){
    
        //print actual memory address
        // cout << page_num*NUM_ELEM_PER_PAGE + i << ": ";
        int val;
        memcpy(&val, &mem[sizeof(int)*(page_num*NUM_ELEM_PER_PAGE + i)], sizeof(int));
        // cout << mem[sizeof(int)*(page_num*NUM_ELEM_PER_PAGE + i)] << " ";
        cout << val << " ";
    }
    cout << endl;
    return;

}

void mem_Block::printFreeList(){

    if (free_list.empty()){
        cout << "Free list is empty. No free pages in the memory!" << endl;
        return;
    }

    cout << "Free List: " << endl;
    priority_queue<int , vector<int>, greater<int>> temp = free_list;

    while (!temp.empty()){
        cout << temp.top() << " ";
        temp.pop();
    }
    cout << endl;
    return;
}

void Sym_Table::init(){

    first_free_entry = 0;
    num_free_entries = NUM_SYM_TABLE_ENTRIES;

    list_map.clear();  // reinitialize the map to empty

    // initialize entries
    entries = (Sym_Table_Entry *)malloc(NUM_SYM_TABLE_ENTRIES*sizeof(Sym_Table_Entry));

    
    for ( size_t i = 0; i < NUM_SYM_TABLE_ENTRIES-1; i++){

        entries[i].next_entry = i + 1;
        entries[i].size = 0;
        entries[i].list_pages = new vector<unsigned long>();
        entries[i].capacity = 0;
        
    }  
    entries[NUM_SYM_TABLE_ENTRIES - 1].next_entry = -1;
 
}


int Sym_Table::addEntry(int scope, string list_name, vector<unsigned long> page_mapping, unsigned long capacity){
    
    if (num_free_entries == 0){
            cout << "Error: No more free entries in symbol table! Cannot create the list!" << endl;
            return -1;
    }

    // if an entry already exists for the same scope and list name, return error
    if (list_map.find(make_pair(scope, list_name)) != list_map.end()){
        cout << "Error: Cannot create two lists with same name within the same scope!" << endl;
        return -1;
    }

    unsigned long entry_num = first_free_entry;

    first_free_entry = entries[first_free_entry].next_entry;
    num_free_entries--;

    // add entry to the map
    list_map[make_pair(scope, list_name)] = entry_num;

    entries[entry_num].next_entry = -1;
    *entries[entry_num].list_pages = page_mapping;
    entries[entry_num].size = 0;
    entries[entry_num].capacity = capacity;


    return entry_num;

}

void Sym_Table::deleteEntry(int scope, string list_name){

    // if an entry does not exist for the same scope and list name, return error
    if (list_map.find(make_pair(scope, list_name)) == list_map.end()){
        cout << "Error: No entry in the Symbol table for the given list_name and scope!" << endl;
        return;
    }

    unsigned long entry_num = list_map[make_pair(scope, list_name)];
    list_map.erase(make_pair(scope, list_name));

    entries[entry_num].next_entry = first_free_entry;
    first_free_entry = entry_num;
    // reset the vector (page_mapping) to empty
    entries[entry_num].list_pages->clear();
    entries[entry_num].capacity = 0;
    entries[entry_num].size = 0;

    num_free_entries++;
    return;

}

void Sym_Table::printEntry(int scope, string list_name){
    
        // if an entry does not exist for the same scope and list name, return error
        if (list_map.find(make_pair(scope, list_name)) == list_map.end()){
            cout << "Error: No entry in the Symbol table for the given list_name and scope!" << endl;
            return;
        }
    
        unsigned long entry_num = list_map[make_pair(scope, list_name)];

        cout << "Scope: " << scope << endl;
        cout << "List name: " << list_name << endl;
        cout << "Page mapping: " << endl;

        for (size_t i = 0; i < entries[entry_num].list_pages->size(); i++){
            cout << (*entries[entry_num].list_pages)[i] << " ";
        }
        cout << endl;
        return;
}

void Sym_Table::printSymTable(){
   

    // if the symbol table is empty, print error message and return
    if (list_map.empty()){
        cout << "Symbol table is empty!" << endl;
        return;
    }

    cout << "Symbol Table: " << endl;
    cout << "Size of the symbol table: " << list_map.size() << endl;

    for (auto it = list_map.begin(); it != list_map.end(); it++){
        cout << "------------------------------------------------------------" << endl;
        printEntry(it->first.first, it->first.second);
        cout << "------------------------------------------------------------" << endl;
    }
    cout << endl;
}

void initScope(){
    curr_scope++;
}

void endScope(){

    // check anfd delete all the entries in the list_map for the current scope
    int flag = 0;
    int ctr = 0;

    for (auto it = list_map.begin(); it != list_map.end(); it++){
        if (it->first.first == curr_scope){

            // cout << "Deleting entry for list: " << it->first.second << " and scope: " << it->first.first << endl;
            sym_table->deleteEntry(it->first.first, it->first.second);
            
            // cout << "Deleted entry for list: " << it->first.second << " and scope: " << it->first.first << endl;
            ctr++;
            break;
        }

    
    }

    // cout << "curr_scope: " << curr_scope << endl;
    if (curr_scope == 0){
        cout << "Max page usage: " << MAX_PAGES_USED << endl;
    }

    curr_scope--;

}

int createList(size_t size, string list_name){

    // check if there is any free entry in Symbol tabel to create a list
    if (sym_table->num_free_entries == 0){
        cout << "Error: Symbol table has no free entry, cannot create a new list!" << endl;
        return -1;
    }

    // if an entry already exists for the same scope and list name, return error
    if (list_map.find(make_pair(curr_scope, list_name)) != list_map.end()){
        cout << "Error: Cannot create two lists with same name within the same scope!" << endl;
        return -1;
    }

    // check if there is enough free no of pages to accomodate the list
    size_t num_pages_req = (size_t)ceil((double)(size*sizeof(int))/(double)PAGE_SIZE);
    // cout << "num_pages_req: " << num_pages_req << endl;

    if (mem->num_free_pages < num_pages_req){
        cout << "Error: Sufficient memory space not available, cannot create the list!"<<endl;
        return -1;
    }

    else {
        size_t pages_alloted = 0;

        // create a vector to store the page mapping
        vector<unsigned long> page_mapping;

        // allocate pages to the list
        while(1){

            if (pages_alloted == num_pages_req){
                break;
            }
            unsigned long page_num = mem->free_list.top();
            mem->free_list.pop();
            // mem->num_free_pages--;
            // cout << page_num << endl;
            page_mapping.push_back(page_num);
            pages_alloted++;
        }
        // cout << "pages_alloted: " << pages_alloted << endl;

        // update the number of free pages
        mem->num_free_pages -= pages_alloted;

        MAX_PAGES_USED = (mem->capacity/PAGE_SIZE) - mem->num_free_pages;

        // create a new entry in the Symbol table
        unsigned long entry_num = sym_table->addEntry(curr_scope, list_name, page_mapping, size);
        if (entry_num == -1){
            cout << "Error: Cannot create a new entry in the Symbol table!" << endl;
            return -1;
        }

        return 1;  
    }
}

void assignVal(string list_name, unsigned long offset, int *value){


    // if an entry exists for the same scope and list name, get the entry number
    if (list_map.find(make_pair(curr_scope, list_name)) != list_map.end()){

        unsigned long entry_num = list_map[make_pair(curr_scope, list_name)];

        // compute the actual address
        unsigned long address = compute_mem_address(entry_num, offset);

        // assign the value to the address
        // cout << "address: " << address << endl;
        memcpy(&mem->mem[address], value, sizeof(int));
        // cout << "val: "<< mem->mem[address] << endl;

        // cout << "entry num: " << entry_num << endl;
        sym_table->entries[entry_num].size += 1;
        // cout << "list size: " << sym_table->entries[entry_num].size << endl;
        
    }

    // search in the global scope
    else if (list_map.find(make_pair(0, list_name)) != list_map.end()){

        unsigned long entry_num = list_map[make_pair(0, list_name)];

        // compute the actual address
        unsigned long address = compute_mem_address(entry_num, offset);

        // assign the value to the address
        int val = *value;
        memcpy( (void *)(mem->mem + address), (void *)&val, sizeof(int) );

        sym_table->entries[entry_num].size += 1;

    }
    
    else {
        cout << "Error: No entry for list in the Symbol table! List is invalid!" << endl;
        return;
    }
    
    return;
}


// unsigned long compute_mem_address(unsigned long entry_num, unsigned long offset){
      
//     // check if the offset is within the list size
//     if (offset >= sym_table->entries[entry_num].capacity){
//         cout << "Error: Offset is out of bounds!" << endl;
//         return -1;
//     }


//     // get the page number and offset within the page
//     unsigned long page_num = (offset*sizeof(int))/PAGE_SIZE;
//     unsigned long offset_within_page = (offset)%(PAGE_SIZE/sizeof(int));

    
//     unsigned long abs_page_num = 0;

//             // get the page number from the page mapping
//     unsigned long start_page = sym_table->entries[entry_num].list_pages[0].first;
//     unsigned long temp = sym_table->entries[entry_num].list_pages[0].second;

//     int i = 0;
//     while(1){
//         if (page_num < temp){
//             abs_page_num = start_page + page_num;
//             break;
//         }
//         else {
//             page_num -= temp;
//             start_page = temp;
//             temp = sym_table->entries[entry_num].list_pages[++i].second;
//         }
//     }
//     cout << "abs_page_num: " << abs_page_num << endl;
//     cout << "offset_within_page: " << offset_within_page << endl;

//     // compute the actual address
//     unsigned long address = (abs_page_num*PAGE_SIZE) + offset_within_page*sizeof(int);
//     cout << "address: "<<address<<endl;

//     return address;
// }

unsigned long compute_mem_address(unsigned long entry_num, unsigned long offset){
      
    // check if the offset is within the list size
    if (offset >= sym_table->entries[entry_num].capacity){
        cout << "Error: Offset is out of bounds!" << endl;
        return -1;
    }
    // cout << "offset: " << offset << endl;
    
    // compute the actual address
    unsigned long page_num = (offset*sizeof(int))/PAGE_SIZE;
    unsigned long offset_within_page = (offset*sizeof(int))%(PAGE_SIZE);

    // cout << "page_num: " << page_num << endl;
    // cout << "offset_within_page: " << offset_within_page << endl;
    unsigned long address = (*sym_table->entries[entry_num].list_pages)[page_num]*PAGE_SIZE + offset_within_page;
    return address;
}


void getVal(string list_name, unsigned long offset, int *value){

    // if an entry exists for the same scope and list name, get the entry number
    if (list_map.find(make_pair(curr_scope, list_name)) != list_map.end()){

        unsigned long entry_num = list_map[make_pair(curr_scope, list_name)];
        
        // compute the actual address
        unsigned long address = compute_mem_address(entry_num, offset);

        memcpy(value, &mem->mem[address], sizeof(int));
    }

    else if (list_map.find(make_pair(0, list_name)) != list_map.end()){

        unsigned long entry_num = list_map[make_pair(0, list_name)];

        // compute the actual address
        unsigned long address = compute_mem_address(entry_num, offset);

        memcpy(value, &mem->mem[address], sizeof(int));
    }

    else {
        cout << "Error: No entry for list in the Symbol table! List is invalid!" << endl;
        return;
    }
    
    return;
}

void freeList(string listname){

    // search for the list in the current scope
    if (list_map.find(make_pair(curr_scope, listname)) != list_map.end()){

        unsigned long entry_num = list_map[make_pair(curr_scope, listname)];

        list_map.erase(make_pair(curr_scope, listname));

        sym_table->entries[entry_num].next_entry = sym_table->first_free_entry;
        sym_table->first_free_entry = entry_num;

        // cout << "Num free pages in mem before freeing: " << mem->free_list.size() << endl;

        // push the pages occupiesd by the list to the free list
        for (int i = 0; i < sym_table->entries[entry_num].list_pages->size(); i++){
            mem->free_list.push(sym_table->entries[entry_num].list_pages->at(i));
        }
        // cout << "Num free pages in mem after freeing: " << mem->free_list.size() << endl;

        mem->num_free_pages += sym_table->entries[entry_num].list_pages->size();
        // delete the list pages vector
        sym_table->entries[entry_num].list_pages->clear();

        sym_table->entries[entry_num].capacity = 0;
        sym_table->entries[entry_num].size = 0;    

        sym_table->num_free_entries++;    
    }

    else {
        // cout << "Error: No entry for list in the Symbol table! List is invalid!" << endl;
        return;
    }
}

void printList(string listname){

    if (list_map.find(make_pair(curr_scope, listname)) != list_map.end()){

        unsigned long entry_num = list_map[make_pair(curr_scope, listname)];

        vector <unsigned long> pages = *sym_table->entries[entry_num].list_pages;
        for (int i = 0; i < pages.size(); i++){
            mem->printMemPage(pages[i]);
        }
    }

    else if (list_map.find(make_pair(0, listname)) != list_map.end()){

        unsigned long entry_num = list_map[make_pair(0, listname)];

        vector <unsigned long> pages = *sym_table->entries[entry_num].list_pages;
        for (int i = 0; i < pages.size(); i++){
            mem->printMemPage(pages[i]);
        }
    }

    else {
        cout << "Error: No entry for list in the Symbol table! List is invalid!" << endl;
        return;
    }
}
