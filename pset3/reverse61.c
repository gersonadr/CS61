#include "io61.h"

// Usage: ./reverse61 [-s SIZE] [FILE]
//    Copies the input FILE to standard output one character at a time,
//    reversing the order of characters in the input.

int main(int argc, char** argv) {
    // Parse arguments
    ssize_t inf_size = -1;
    while (argc >= 3) {
        if (strcmp(argv[1], "-s") == 0) {
            inf_size = (ssize_t) strtoul(argv[2], 0, 0);
            argc -= 2, argv += 2;
        } else
            break;
    }

    // Open files, measure file sizes
    const char* in_filename = argc >= 2 ? argv[1] : NULL;
    io61_profile_begin();
    io61_file* inf = io61_open_check(in_filename, O_RDONLY);
    io61_file* outf = io61_fdopen(STDOUT_FILENO, O_WRONLY);

    if (inf_size < 0)
        inf_size = io61_filesize(inf);
    if (inf_size < 0) {
        fprintf(stderr, "reverse61: can't get size of input file\n");
        exit(1);
    }
    if (io61_seek(inf, 0) < 0) {
        fprintf(stderr, "reverse61: input file is not seekable\n");
        exit(1);
    }

    while (inf_size != 0) {
        --inf_size;
        io61_seek(inf, inf_size);
        int ch = io61_readc(inf);
        io61_writec(outf, ch);
    }

    io61_close(inf);
    io61_close(outf);
    io61_profile_end();
}
