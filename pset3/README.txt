README for CS 61 Problem Set 3
------------------------------
YOU MUST FILL OUT THIS FILE BEFORE SUBMITTING!

OTHER COLLABORATORS AND CITATIONS (if any):

none.

KNOWN BUGS (if any):

- Test 12 fails on grading server because it uses mmap to load a 20MB file and I guess the grading server does not have enough memory.
Fixing this problem would require to change the strategy to non-sequencial access to files.

- io61_write_mapped() is using POSIX write which causes check-13 to be worse than stdio. This could be a future improvement.

NOTES FOR THE GRADER (if any):

I chose to implement single block cache, after notifing multi-block made the code complex and slower by 30%.
After re-writing this assignment 3 times (first with single block as the Roadmap, then multi-block as Margo showed in class, and later to single-block + mmap), I discovered that, 
other than minimizing read/write IO operations as humanly as possible, the choice of implementation may affect performance greatly. This is a very good insight, and the reason
I've chosen this course in the first place (able to experiment code structures and see the performance). 
