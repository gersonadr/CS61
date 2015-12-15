README for CS 61 Problem Set 4
------------------------------
OTHER COLLABORATORS AND CITATIONS (if any):

 

NOTES FOR THE GRADER (if any):

 - Group of just me. 

 I chose to implement sets of background processes by creating an idea of "expression" (for lack of better term), which is basically list of commands separated by '&', ';' or End-Of-Line.
 Another approach would be to use signals and a timer to check on child status, but I figured it would introduce more complexity. I trust we will practice signals on future assignments.

 We (class CS61) covered more topics (pipes, fork/execs, signaling 101) on this assignment, which is great. The end program is not as complex as I initially thought it would be.
 Asking on Piazza early and often (specially test-23) helped to take good directions (such as previous point) that made adding features to my shell a breeze. =) Thanks!