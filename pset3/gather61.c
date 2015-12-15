#include "io61.h"

// Usage: ./gather61 [-b BLOCKSIZE] [FILE1 FILE2...]
//    Copies the input FILEs to standard output, alternating between
//    FILEs with every block. (I.e., read a block from FILE1, then
//    a block from FILE2, etc.) This is a "gather" I/O pattern: many
//    input files are gathered into a single output file.
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
    const char** in_filenames =
        (const char**) calloc(nfiles, sizeof(const char*));
    for (int i = 0; i < nfiles && i + 1 < argc; ++i)
        in_filenames[i] = argv[i + 1];
    io61_profile_begin();
    io61_file** infs = (io61_file**) calloc(nfiles, sizeof(io61_file*));
    for (int i = 0; i < nfiles; ++i)
        infs[i] = io61_open_check(in_filenames[i], O_RDONLY);
    io61_file* outf = io61_fdopen(STDOUT_FILENO, O_WRONLY);

    // Copy file data
    int whichf = 0, ndeadfiles = 0;
    while (ndeadfiles != nfiles) {
        if (infs[whichf]) {
            ssize_t amount = io61_read(infs[whichf], buf, blocksize);
            if (amount <= 0) {
                io61_close(infs[whichf]);
                infs[whichf] = NULL;
                ++ndeadfiles;
            } else
                io61_write(outf, buf, amount);
        }
        whichf = (whichf + 1) % nfiles;
    }

    io61_close(outf);
    io61_profile_end();
    free(infs);
    free(in_filenames);
    free(buf);
}
