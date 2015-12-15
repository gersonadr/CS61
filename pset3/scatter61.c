#include "io61.h"

// Usage: ./scatter61 [-b BLOCKSIZE] [FILE1 FILE2...]
//    Copies the standard input to the FILEs, alternating between FILEs
//    with every block. (I.e., write a block to FILE1, then
//    a block to FILE2, etc.) This is a "scatter" I/O pattern: one
//    input file is scattered into many output files.
//    Default BLOCKSIZE is 1.

int main(int argc, char** argv) {
    // Parse arguments
    size_t blocksize = 1;
    if (argc >= 3 && strcmp(argv[1], "-b") == 0) {
        blocksize = strtoul(argv[2], 0, 0);
        argc -= 2, argv += 2;
    }

    // Allocate buffer, open files
    assert(blocksize > 0);
    char* buf = (char*) malloc(blocksize);

    int nfiles = argc < 2 ? 1 : argc - 1;
    const char** out_filenames =
        (const char**) calloc(nfiles, sizeof(const char*));
    for (int i = 0; i < nfiles && i + 1 < argc; ++i)
        out_filenames[i] = argv[i + 1];
    io61_profile_begin();
    io61_file* inf = io61_fdopen(STDIN_FILENO, O_RDONLY);
    io61_file** outfs = (io61_file**) calloc(nfiles, sizeof(io61_file*));
    for (int i = 0; i < nfiles; ++i)
        outfs[i] = io61_open_check(out_filenames[i],
                                   O_WRONLY | O_CREAT | O_TRUNC);

    // Copy file data
    int whichf = 0;
    while (1) {
        ssize_t amount = io61_read(inf, buf, blocksize);
        if (amount <= 0)
            break;
        io61_write(outfs[whichf], buf, amount);
        whichf = (whichf + 1) % nfiles;
    }

    io61_close(inf);
    for (int i = 0; i < nfiles; ++i)
        io61_close(outfs[i]);
    io61_profile_end();
    free(outfs);
    free(out_filenames);
    free(buf);
}
