/*
 * Harvard Extension School - CSCIE-61
 * 
 * Problem Set #1
 * Student: Gerson A. D. Rodrigues
 * HID: 20787139
 */
#define M61_DISABLE 1
#include "m61.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <assert.h>

/* Statistics about the memory allocator */
struct m61_statistics stats;

/* Head of m61_node's list 
 * 
 * This list contains metadata about blocks of memory used by the allocator,
 * such as block size, pointer and status
 */
struct m61_node * head = NULL;

/**
 * Allocate sz bytes of memory from the heap
 *
 * @param sz - number of bytes to be allocated
 * @param file - caller's program file name
 * @param line - caller's program line number
 * @return pointer - location in heap which marks the beginning of allocated space
 */
void* m61_malloc(size_t sz, const char* file, int line) {
    (void) file, (void) line;   // avoid uninitialized variable warnings
    
    //avoid overflowing the sz variable
    if (sz >= (size_t) - sizeof(struct m61_node) + NUM_VERIFICATION_CHARS) {
        stats.nfail += 1;
    	stats.fail_size += sz;
        return NULL;
    }
    
    //Calls native allocator for size + appended verification bytes
    char * data = (char*) malloc(sz + NUM_VERIFICATION_CHARS);
    
    //If a problem occurred (eg: heap full), mark as fail
    if (data == NULL){
    	stats.nfail += 1;
    	stats.fail_size += sz;
    	return NULL;
    }
    else {
        //Mark statistics as success
    	stats.nactive += 1;
    	stats.ntotal += 1;
        stats.active_size += sz;
        stats.total_size += sz;
        
        //Updates the heap_min and heap_max addresses
        if (!stats.heap_min || stats.heap_min > data)
        	stats.heap_min = data;
    
        if (!stats.heap_max || stats.heap_max < data)
        	stats.heap_max = data + sz + (NUM_VERIFICATION_CHARS * sizeof(size_t));
    }
    
    //Appends the verification digits at the end of memory block, to detect wild writes
    m61_append_verification_chars(data, sz);
    //Adds one node to list of allocated memory blocks
    m61_add_to_list(data, sz, MEM_ALLOC, (char*) file, line);
    
    return data;
}

/**
 * Appends pre-defined verification characters at the end of memory block
 * This will be used by "m61_free" to detect wild writes problem.
 *
 * @param data - pointer to beginning of allocated memory 
 * @param size - size of allocated memory
 */
void m61_append_verification_chars(char* data, size_t sz){
    
    for (int i = sz; i < (int)sz + NUM_VERIFICATION_CHARS; i++){
        data[i] = END_OF_BLOCK;
    }
}

/**
 * Adds a node with memory allocation metadata to the .
 *
 * @param ptr - pointer to beginning of allocated memory 
 * @param size - size of allocated memory
 * @param status - status (MEM_ALLOC or MEM_FREE)
 * @param file - caller's program file name
 * @param line - caller's program line number
 */
void m61_add_to_list(void* ptr, size_t size, int status, char* file, int line){

    //If list is empty
    if (head == NULL){
        head = malloc(sizeof(struct m61_node));
        
        head->is_valid = TRUE;
        head->pointer = ptr;
        head->size = size;
        head->status = status;
        head->file = file;
        head->line = line;
        head->next = NULL;
        head->prev = NULL;
    }
    else {
        struct m61_node * new_node = malloc(sizeof(struct m61_node));
        
        new_node->is_valid = TRUE;
        new_node->pointer = ptr;
        new_node->size = size;
        new_node->status = status;
        new_node->file = file;
        new_node->line = line;
        
        //Gets last position in list
        struct m61_node* temp = head;
        while (temp->next != NULL){
            temp = temp->next;
        }
        //Bi-directionally points the new node to the last item
        temp->next = new_node;
        new_node->prev = temp;
        new_node->next = NULL;
    }
    
}

/**
 * Releases a block of previously allocated memory from the heap,
 * making it available to use by other 
 *
 * @param ptr - pointer to beginning of allocated memory 
 * @param file - caller's program file name
 * @param line - caller's program line number
 */
void m61_free(void *ptr, const char *file, int line) {
    (void) file, (void) line;   // avoid uninitialized variable warnings
    
    //Trivial case
    if (ptr == NULL){
        return;
    }
    
    //If pointer is not in heap range, returns error
    if (!m61_check_pointer_in_heap(ptr)){
        fprintf( stderr, "MEMORY BUG: %s:%d: invalid free of pointer %p, not in heap\n", file, line, ptr);
        abort();
    }
    //If pointer was not previously allocated and list of nodes was not overflown, returns error
    if (head->is_valid && !m61_check_pointer_status(ptr, MEM_ALLOC)){
        fprintf(stderr, "MEMORY BUG: %s:%d: invalid free of pointer %p, not allocated\n", file, line, ptr);
        
        //If the pointer is contained inside an allocated block of memory, shows detailed error message
        struct m61_node * closest = m61_find_node_with_closest_pointer(ptr);
        if (closest != NULL){
            fprintf(stderr, "  %s:%d: %p is %d bytes inside a %d byte region allocated here\n", 
                    closest->file, closest->line, ptr, ptr - closest->pointer, closest->size);
        }
        abort();
    }
    //If pointer is already free, returns error
    else if (m61_check_pointer_status(ptr, MEM_FREE)){
        fprintf(stderr, "MEMORY BUG: %s:%d: invalid free of pointer %p\n", file, line, ptr);
        abort();
    }
    //If write operation is detected outside the boundaries of the memory block, returns error
    if (!m61_check_wild_write(ptr)){
        fprintf( stderr, "MEMORY BUG: %s:%d: detected wild write during free of pointer %p\n", file, line, ptr);
        abort();
    }
    
    //In case of success, updates the stats..
    stats.active_size -= m61_get_pointer_size(ptr);
    stats.nactive -= 1;
    
    //..frees the memory calling native's free,
    free(ptr);
    
    //..and mark the node as status = MEM_FREE
    struct m61_node * node = m61_find_node(ptr);
    node->status = MEM_FREE;
}

/**
 * Iterates over list of nodes, trying to find a block of allocated memory 
 * such that the pointer is contained whitin the block 
 *
 * @param ptr - pointer to any memory location
 * @return node - node that contains pointer, or NULL if no such node was found
 */
struct m61_node * m61_find_node_with_closest_pointer(void* ptr){
    
    struct m61_node * closest = head;
    
    while (closest != NULL){
        if (ptr >= closest->pointer && ptr <= closest->pointer + closest->size){
            return closest;
        }
        closest = closest->next;
    }
    return NULL;
}

/**
 * Validates if pointer is referencing a valid position in heap memory space
 *
 * @param ptr - pointer to any location in heap
 * @return result - TRUE or FALSE
 */
int m61_check_pointer_in_heap (void *ptr){
    int result = FALSE;
    
    char* pointer = (char*) ptr;
    
    //If heap was initialized
    if (stats.heap_min != NULL && stats.heap_max != NULL){
        
        //If ptr is within heap range
        if (stats.heap_min <= pointer && stats.heap_max >= pointer){
            result = TRUE;
        }
    }

    return result;
}

/**
 * Retrieves the node corresponding to pointer argument and check if 
 * node status is as expected
 *
 * @param ptr - pointer to heap memory block
 * @param status - expected status (MEM_FREE or MEM_ALLOC)
 * @return result - TRUE or FALSE
 */
int m61_check_pointer_status (void *ptr, int status){
    int result = FALSE;
    
    struct m61_node * node = m61_find_node(ptr);
    if (node != NULL && node->is_valid){
        if (node->status == status){
            return TRUE;
        }
    }

    return result;
}

/*
 * Iterates over list of nodes, from last to first (more recent to last recent),
 * looking for a node which matches to pointer
 *
 * @param ptr - pointer to memory  block
 * @param status - expected status (MEM_FREE or MEM_ALLOC)
 * @return result - TRUE or FALSE
 */
struct m61_node * m61_find_node(void *ptr){

    struct m61_node * current = head;
    
    while (current->next != NULL){
        current = current->next;
    }
    
    while (current != NULL){
        if (current->pointer == ptr){
            return current;
        } else {
            current = current->prev;
        }
    }
    return NULL;
}

/*
 * Check if verification characters are present at the
 * memory blocks end. If not present, a wild write occurred.
 *
 * @param ptr - pointer to memory  block
 * @return result - TRUE or FALSE
 */
int m61_check_wild_write(void *ptr){
    int result = TRUE;
    
    //Check if node list was overflown
    if (!head->is_valid){
        result = FALSE;
    }
    else {
        //Reads the size+n position,
        char* pointer = (char *) ptr;
        int memory_block_size = m61_get_pointer_size(ptr);
     
        //.. if verification chars are not present, flag an error
        if (pointer[memory_block_size] != END_OF_BLOCK) {
            result = FALSE;
        }
    }
    return result;
}

/*
 * Reallocates an existing memory block with a new size, copying data
 * from source to destination.
 *
 * @param ptr - pointer to source memory block
 * @param sz - number of bytes to be copied
 * @param file - caller's program file name
 * @param line - caller's program line number
 * @return new_pointer - pointer to destination memory block
 */
void* m61_realloc(void* ptr, size_t sz, const char* file, int line) {
    void* new_ptr = NULL;
    if (sz)
        new_ptr = m61_malloc(sz, file, line);
    //If source and destination pointers are valid
    if (ptr && new_ptr) {
        //..retrieves the source size,
        size_t ptr_size = m61_get_pointer_size(ptr);
        //copy N bytes from source to destination
        memcpy(new_ptr, ptr, ptr_size);
        //append verification chars at the end of destination
        m61_append_verification_chars(new_ptr, sz);
    }
    //Releases the source from memory
    m61_free(ptr, file, line);
    
    return new_ptr;
}

/*
 * Retrieves the size of allocated memory pointed by ptr.
 *
 * @param ptr - pointer to memory block
 * @return size - number of bytes allocated in memory 
 */
size_t m61_get_pointer_size(void* ptr){
    
    struct m61_node *current = m61_find_node(ptr);
    if (current != NULL){
        return current->size;
    }
    else{
        return 0;
    }
}

/*
 * Allocates 'nmemb' blocks of memory with size 'sz' and sets the content to 0.
 *
 * @param nmemb - number of blocks of memory
 * @param sz - size of each block of memory
 * @param file - caller's program file name
 * @param line - caller's program line number
 * @return pointer - pointer to newly allocated block of memory 
 */
void* m61_calloc(size_t nmemb, size_t sz, const char* file, int line) {
    
    // Avoids overflowing the size_t variable.
    //If error occurrs, add fail to statistics
    if (sz != 0 && nmemb > ((size_t)-1 / sz)){
        stats.nfail += 1;
    	stats.fail_size += (sz * nmemb);
        return NULL;
    } 
    
    //Allocates the memory
    void* ptr = m61_malloc(nmemb * sz, file, line);
    if (ptr)
        //Sets all bytes to 0
        memset(ptr, 0, nmemb * sz);
    return ptr;
}

/*
 * Gets statistic data from global variable and assigns to reference parameter
 *
 * @param _stats - statistics reference parameter
 */
void m61_getstatistics(struct m61_statistics* _stats) {
    _stats->active_size = stats.active_size;
    _stats->fail_size = stats.fail_size;
    _stats->heap_max = stats.heap_max;
    _stats->heap_min = stats.heap_min;
    _stats->nactive = stats.nactive;
    _stats->nfail = stats.nfail;
    _stats->ntotal = stats.ntotal;
    _stats->total_size = stats.total_size;
}

/*
 * Prints statistics on screen on pre-defined format.
 *
 * @param _stats - statistics reference parameter
 */
void m61_printstatistics(void) {
    struct m61_statistics stats;
    m61_getstatistics(&stats);

    printf("malloc count: active %10llu   total %10llu   fail %10llu\n",
           stats.nactive, stats.ntotal, stats.nfail);
    printf("malloc size:  active %10llu   total %10llu   fail %10llu\n",
           stats.active_size, stats.total_size, stats.fail_size);
}

/*
 * Searches the list of nodes for active blocks of memory 
 * that can cause memory leak, and prints them on screen.
 */
void m61_printleakreport(void) {
    if(head != NULL)
    {
        struct m61_node * current = head;
        
        while(current != NULL)
        {
            if(current->status == MEM_ALLOC)
                printf("LEAK CHECK: %s:%d: allocated object %p with size %d\n", current -> file, current -> line, current -> pointer, current -> size);

            current = current -> next;
        }
    }
}


