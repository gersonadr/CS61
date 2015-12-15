#include "io61.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>

#define MAX_CACHE_SIZE 7000

#define ACCESS_SEQ 1
#define ACCESS_RAND 2

#define TRUE 1
#define FALSE 0


// io61.c
// Custom IO implementation using single-block cache and in-memory mapping as caching techniques.

// io61_filedata
//    Data structure that contains cached data (via array of bytes OR mapped file but never both),
//    and cache indexes, such as:
//      - cache size: total amount of slots available for caching
//      - cache index: current valid position in cache
//      - access mode: sequencial or random data access. 
//                     Sequencial access is set by default, when caller opens the file using io61_open, 
//                     and do io61_read or io61_writes without changing the cursor position using io61_seek.
//
//                     If the io61_seek function gets called, access_mode changes to random, in which case the
//                     opened file is mmaped into memory (mmap) to allow reads/writes for scatter and reverse. 
//
//                     By using mmap, I don't assume anything about how the file will be accessed by caller.
//                     Otherwise I would have to control caches before and after cursor, have side cases for
//                     end or beginning of file (for reverse), and would have to implement multiple cache blocks
//                     for scatter access, and fine-tune the cache block size at each call, which is possible but 
//                     resulted in a much slower implementation. 
//                     (30% slower using multi-blocks cache then single cache block and mmap)
//                     
struct io61_filedata {

    char buf[MAX_CACHE_SIZE];
    char* map;
    
    int cache_size;
    int cache_index;
    int access_mode;
};

// io61_file
//    Data structure for io61 file wrappers.
//    I added file size, cursor_position and a struct containing cache information.
struct io61_file {
    int fd;

    int mode;
    int size;
    int cursor_pos;
    
    struct io61_filedata filedata;
};

ssize_t io61_read_mapped(io61_file* f, char* buf, size_t sz);
ssize_t io61_read_cached_block(io61_file* f, char* buf, size_t sz);
 
ssize_t io61_write_cached_block(io61_file* f, const char* buf, size_t sz);
ssize_t io61_write_mapped(io61_file* f, const char* buf, size_t sz);

// io61_fdopen(fd, mode)
//    Return a new io61_file that reads from and/or writes to the given
//    file descriptor `fd`. `mode` is either O_RDONLY for a read-only file
//    or O_WRONLY for a write-only file. You need not support read/write
//    files.

io61_file* io61_fdopen(int fd, int mode) {
    assert(fd >= 0);
    io61_file* f = (io61_file*) malloc(sizeof(io61_file));
    f->fd = fd;
    (void) mode;
    f->mode = mode;
    f->size = io61_filesize(f);
    
    //Sets sequencial access as default for reads/writes.
    f->filedata.access_mode = ACCESS_SEQ;
    return f;
}


// io61_close(f)
//    Close the io61_file `f` and release all its resources, including
//    any buffers.

int io61_close(io61_file* f) {
    io61_flush(f);
    int r = close(f->fd);
    free(f);
    return r;
}


// io61_readc(f)
//    Read a single (unsigned) character from `f` and return it. Returns EOF
//    (which is -1) on error or end-of-file.

int io61_readc(io61_file* f) {

    struct io61_filedata * filedata = &f->filedata;

    // To improve speed, try to read from cache (if there is any)...
    if (filedata->cache_index + 1 < filedata->cache_size ) {
        //casting to unsigned char to avoid 'EOF' characters in binary file
        return (unsigned char)filedata->buf[filedata->cache_index++];
    }
    // ... otherwise, calls io61_read to populate and read from cache
    else{
        char * buf = (char*) malloc(sizeof(int));
        if (io61_read(f, buf, 1) == 1)
            return (unsigned char)buf[0];
        else
            return EOF;
    }
}

// io61_read(f, buf, sz)
//    Read up to `sz` characters from `f` into `buf`. Returns the number of
//    characters read on success; normally this is `sz`. Returns a short
//    count if the file ended before `sz` characters could be read. Returns
//    -1 an error occurred before any characters were read.

ssize_t io61_read(io61_file* f, char* buf, size_t sz) {

    // If access mode = Sequencial, use cache blocks
    if (f->filedata.access_mode == ACCESS_SEQ){
        return io61_read_cached_block(f, buf, sz);
    // If access mode = Random, use memory mapped file
    } else {
        return io61_read_mapped(f, buf, sz);
    }
}

// io61_read_cached_block(f, buf, sz)
//    This method implements single-block cache read operations.
//    It first tries to populates the cache (if empty). If there cache available, 
//    reads 'sz bytes from cache block and copies to 'buf' and returns the number of copied bytes. 
//    If 'sz' > number of cached slots, copy the available cache slots to 'buf' and reads the remaining
//    number of bytes from IO directly. 

ssize_t io61_read_cached_block(io61_file* f, char* buf, size_t sz) {

    struct io61_filedata * fdata = &f->filedata;

    size_t nread = 0;
    
    // If cache is empty or all bytes were read, populates from IO
    if (fdata->cache_index >= fdata->cache_size){
        fdata->cache_size = read(f->fd, fdata->buf, MAX_CACHE_SIZE);
        fdata->cache_index = 0;
    }
    
    int remaining = 0;
    // If cache size is big enough to read 'sz' bytes, copy to 'buf'
    // and updates cache index...
    if ((int)sz <= (fdata->cache_size - fdata->cache_index)){
        memcpy(buf, &fdata->buf[fdata->cache_index], sz);
        nread = sz;
        fdata->cache_index += sz;
    } 
    // ... if not, copy to 'buf' the available cached slots, 
    // and reads the remaining from IO 
    else if ((remaining = fdata->cache_size - fdata->cache_index)) {
        memcpy(buf, &fdata->buf[fdata->cache_index], remaining);
        nread = read(f->fd, buf + remaining, sz-remaining) + remaining;
        fdata->cache_index += sz;
    }
    
    if (nread != 0 || sz == 0 || io61_eof(f))
        return nread;
    else
        return -1;
}

// io61_read_mapped(f, buf, sz)
//    This method implements read with cached file in-memory for random access reads.
//
//    First time this method is called, it maps the file into memory using 'mmap' (lazy loading technique) 
//    All subsequent calls will copy 'sz' bytes from in-memory file into 'buf' and return number of copied bytes.
//
//    NOTE: There is a limitation for mmap for files as large as 20MB, but since it simplifies the caching
//    for reverse and scatter reads, I overlooked this problem as explained above.
//
ssize_t io61_read_mapped(io61_file* f, char* buf, size_t sz) {

    struct io61_filedata * fdata = &f->filedata;
    
    // Will load the file into virtual memory once per file
    if (fdata->map == NULL) {
        int nbytes_to_read = f->size > 0 ? f->size : MAX_CACHE_SIZE;
        fdata->map = mmap(NULL, nbytes_to_read, PROT_READ, MAP_PRIVATE, f->fd, 0);
    }
    // Copies 'sz' bytes from file, begining at cursor position
    memcpy(buf, &fdata->map[f->cursor_pos], sz);
    return sz;
}

// io61_writec(f)
//    Write a single character `ch` to `f`. Returns 0 on success or
//    -1 on error.
//  NOTE: Similar logic to io61_readc()

int io61_writec(io61_file* f, int ch) {
    struct io61_filedata * fdata = &f->filedata;
    
    // To improve speed, try to write to cache (if slots are available)...
    if (fdata->cache_index + 1 < fdata->cache_size){
        fdata->buf[fdata->cache_index++] = ch;
        return 0;
    }
    // ... otherwise, calls io61_write to write to cache block or IO
    else {
        char * buf = (char*) malloc(sizeof(int));
        buf[0] = ch;
        if (io61_write(f, buf, 1) == 1)
            return 0;
        else
            return -1;
    }
}

// io61_write(f, buf, sz)
//    Write `sz` characters from `buf` to `f`. Returns the number of
//    characters written on success; normally this is `sz`. Returns -1 if
//    an error occurred before any characters were written.
//    
//    NOTE: similar to rio61_read, can either write to single-block cache if access mode = Sequencial,
//    or write to in-memory mapped file if access mode = Random.

ssize_t io61_write(io61_file* f, const char* buf, size_t sz) {
    struct io61_filedata * fdata = &f->filedata;
    
    // If access mode = Sequencial, use cache blocks
    if (fdata->access_mode == ACCESS_SEQ){
        return io61_write_cached_block(f, buf, sz);
    // If access mode = Random, use memory mapped file
    } else {
        return io61_write_mapped(f, buf, sz);
    }
}

// io61_write_cached_block(f, buf, sz)
//    This method implements single-block cache write operation.
//    - It first checks if cache block is full and flushes to disk (IO write),
//    - If cache is not full, copies 'sz' bytes to cache block and returns the number of copied bytes. 
//    - If 'sz' > number of available cache slots, copies N bytes to cache slots, flushes to disk and writes the remaining bytes to cache blocks
//    Returns the number of written bytes.
ssize_t io61_write_cached_block(io61_file* f, const char* buf, size_t sz) {

    struct io61_filedata * fdata = &f->filedata;
    size_t nwritten = 0;
    
    // If cache is full, ...
    if (fdata->cache_index >= fdata->cache_size){
        // ... flushes cache to disk and ...
        write(f->fd, &fdata->buf, fdata->cache_size);
        
        // ...resets the cache index and size
        fdata->cache_size = MAX_CACHE_SIZE;
        fdata->cache_index = 0;
    }
    
    int remaining = 0;
    // If number of available cache slots is big enough to write 'sz' bytes, 
    // copy from 'buf' to cached block and updates cache index...
    if ((int)sz <= fdata->cache_size - fdata->cache_index){
        memcpy(&fdata->buf[fdata->cache_index], buf, sz);
        nwritten = sz;
        fdata->cache_index += sz;
    } 
    // ... if not, copy N bytes from 'buf' to cache (N < sz), 
    // flushes the cache to disk, and copies the remaining bytes to cache 
    else if ((remaining = fdata->cache_size - fdata->cache_index)) {
        
        memcpy(&fdata->buf[fdata->cache_index], buf, remaining);
        write(f->fd, &fdata->buf, fdata->cache_size);
        nwritten = write(f->fd, &buf[remaining], sz - remaining) + remaining;
        
        // Reset the cache
        fdata->cache_size = 0;
        fdata->cache_index = 0;
    }
    
    if (nwritten != 0 || sz == 0)
        return nwritten;
    else
        return -1;
}

//io61_write_mapped(f, buf, sz)
//   Write 'sz' bytes from 'buf' to file 'f' and return number of written files
//   TO IMPROVE: making direct write IO calls without buffering
ssize_t io61_write_mapped(io61_file* f, const char* buf, size_t sz) {

    lseek(f->fd, f->cursor_pos, SEEK_SET);
    write(f->fd, buf, sz);
    return sz;
}

// io61_flush(f)
//    Forces a write of all buffered data written to `f`.
//    If `f` was opened read-only, io61_flush(f) may either drop all
//    data buffered for reading, or do nothing.

int io61_flush(io61_file* f) {
    (void) f;
    struct io61_filedata * fdata = &f->filedata;
    
    // If mode = Write ...
    if (f->mode == O_WRONLY){
        // ... and access mode = Sequential, flushes cached blocks to disk
        if (fdata->access_mode == ACCESS_SEQ){
            int remaining = 0;
            if ((remaining = fdata->cache_size - fdata->cache_index)) {
                write(f->fd, &fdata->buf, fdata->cache_index);
                fdata->cache_index = fdata->cache_size;
            }
        // ... If access mode = Random, syncs the in-memory file to physical file
        } else {
            msync(fdata->map, f->size, MS_SYNC);
        }
    }
    
    return 0;
}


// io61_seek(f, pos)
//    Change the file pointer for file `f` to `pos` bytes into the file.
//    Returns 0 on success and -1 on failure.
//
//    Also, sets the access mode = Random, because most likely the caller will
//    write / read arbitrary positions on the file, in which case the caching strategy
//    will use in-memory file mapping (mmap).
int io61_seek(io61_file* f, off_t pos) {
    struct io61_filedata * fdata = &f->filedata;
    off_t r = lseek(f->fd, (off_t) pos, SEEK_SET);
    
    if (r == (off_t) pos){
        f->cursor_pos = pos;
        fdata->access_mode = ACCESS_RAND;
        return 0;
    }
    else{
        // Special case: non-seekable files such as /dev/urandom cannot be 'mmapped',
        // therefore cache block is mandatory
        fdata->access_mode = ACCESS_SEQ;
        return -1;
    }
}


// You shouldn't need to change these functions.

// io61_open_check(filename, mode)
//    Open the file corresponding to `filename` and return its io61_file.
//    If `filename == NULL`, returns either the standard input or the
//    standard output, depending on `mode`. Exits with an error message if
//    `filename != NULL` and the named file cannot be opened.

io61_file* io61_open_check(const char* filename, int mode) {
    int fd;
    if (filename)
        fd = open(filename, mode, 0666);
    else if ((mode & O_ACCMODE) == O_RDONLY)
        fd = STDIN_FILENO;
    else
        fd = STDOUT_FILENO;
    if (fd < 0) {
        fprintf(stderr, "%s: %s\n", filename, strerror(errno));
        exit(1);
    }
    return io61_fdopen(fd, mode & O_ACCMODE);
}


// io61_filesize(f)
//    Return the size of `f` in bytes. Returns -1 if `f` does not have a
//    well-defined size (for instance, if it is a pipe).

off_t io61_filesize(io61_file* f) {
    struct stat s;
    int r = fstat(f->fd, &s);
    if (r >= 0 && S_ISREG(s.st_mode) && s.st_size <= SSIZE_MAX)
        return s.st_size;
    else
        return -1;
}


// io61_eof(f)
//    Test if readable file `f` is at end-of-file. Should only be called
//    immediately after a `read` call that returned 0 or -1.

int io61_eof(io61_file* f) {
    char x;
    ssize_t nread = read(f->fd, &x, 1);
    if (nread == 1) {
        fprintf(stderr, "Error: io61_eof called improperly\n\
  (Only call immediately after a read() that returned 0 or -1.)\n");
        abort();
    }
    return nread == 0;
}
