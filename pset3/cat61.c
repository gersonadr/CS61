#include "io61.h"

// Usage: ./cat61 [-s SIZE] [FILE]
//    Copies the input FILE to standard output one character at a time.

int main(int argc, char** argv) {
    // Parse arguments
    size_t inf_size = (size_t) -1;
    while (argc >= 3) {
        if (strcmp(argv[1], "-s") == 0) {
            inf_size = (size_t) strtoul(argv[2], 0, 0);
            argc -= 2, argv += 2;
        } else
            break;
    }

    const char* in_filename = argc >= 2 ? argv[1] : NULL;
    io61_profile_begin();
    io61_file* inf = io61_open_check(in_filename, O_RDONLY);
    io61_file* outf = io61_fdopen(STDOUT_FILENO, O_WRONLY);

    while (inf_size > 0) {
        int ch = io61_readc(inf);
        if (ch == EOF)
            break;
        io61_writec(outf, ch);
        --inf_size;
    }

    io61_close(inf);
    io61_close(outf);
    io61_profile_end();
}
