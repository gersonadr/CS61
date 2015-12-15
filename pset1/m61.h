#ifndef M61_H
#define M61_H 1
#include <stdlib.h>

#define TRUE 1       
#define FALSE 0

#define END_OF_BLOCK '\123'             /* Arbitrary value included at end of memory block. Used to verify wild write errors. */
#define NUM_VERIFICATION_CHARS 2        /* Number of verification characters appended on memory blocks */

#define MEM_ALLOC 1                     /* Node status = ALLOCATED. Means the block of memory is allocated */
#define MEM_FREE 2                      /* Node status = FREE. Means the block of memory is free */
#define NALLOCATORS 40

void* m61_malloc(size_t sz, const char* file, int line);
void m61_free(void* ptr, const char* file, int line);
void* m61_realloc(void* ptr, size_t sz, const char* file, int line);
void* m61_calloc(size_t nmemb, size_t sz, const char* file, int line);

struct m61_statistics {
    unsigned long long nactive;         // # active allocations
    unsigned long long active_size;     // # bytes in active allocations
    unsigned long long ntotal;          // # total allocations
    unsigned long long total_size;      // # bytes in total allocations
    unsigned long long nfail;           // # failed allocation attempts
    unsigned long long fail_size;       // # bytes in failed alloc attempts
    char* heap_min;                     // smallest allocated addr
    char* heap_max;                     // largest allocated addr
};

/* Linked list of memory allocation information */
struct m61_node {
    int is_valid;                       // specifies if node was overriden by external sources
    void* pointer;                      // pointer to block of memory
    size_t size;                        // size of allocated memory in bytes
    int status;                         // status (MEM_ALLOC or MEM_FREE)
    char* file;                         // file name that allocated memory
    int line;                           // file line number that allocated memory
    struct m61_node * next;             // reference to next node
    struct m61_node * prev;             // reference to previous node
};

size_t m61_get_pointer_size(void* ptr);
int m61_check_pointer_status (void *ptr, int status);
int m61_check_pointer_in_heap (void *ptr);
int m61_check_wild_write(void *ptr);

void m61_add_to_list(void* ptr, size_t size, int status, char* file, int line);
void m61_append_verification_chars(char* data, size_t sz);

struct m61_node * m61_find_node(void *ptr);
struct m61_node * m61_find_node_with_closest_pointer(void* ptr);

void m61_getstatistics(struct m61_statistics* stats);
void m61_printstatistics(void);
void m61_printleakreport(void);

#if !M61_DISABLE
#define malloc(sz)              m61_malloc((sz), __FILE__, __LINE__)
#define free(ptr)               m61_free((ptr), __FILE__, __LINE__)
#define realloc(ptr, sz)        m61_realloc((ptr), (sz), __FILE__, __LINE__)
#define calloc(nmemb, sz)       m61_calloc((nmemb), (sz), __FILE__, __LINE__)
#endif

#endif
