#include "sh61.h"
#include <string.h>
#include <errno.h>
#include <regex.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <pthread.h>
#include <dirent.h>
#include <libgen.h>
#include <signal.h>

#define TRUE 1
#define FALSE 0
#define EXIT_SUCCESS 0

#define MAX_COMMANDS_PER_LINE 10
#define LOGICAL_AND 1
#define LOGICAL_OR 2

#define PIPE_READ 0
#define PIPE_WRITE 1
#define PIPE_ERROR 2

// struct command
//    Data structure describing a command. Add your own stuff.
typedef struct command command;
struct command {
    int argc;      // number of arguments
    char** argv;   // arguments, terminated by NULL
    pid_t pid;     // process ID running this command, -1 if none

    int logical_op;   // logical operator on command's right side
    int redirect_op;  // redirect operator on command's right side
    int is_background; // indicates if command should be processed in backgound (& token)
    
    int pipe_in;      // pipe intput indicator. TRUE (1) if command's input comes from pipe 
    int pipe_out;     // pipe output indicator. TRUE (1) if command's output will write to pipe
    int pipefd[3];    // pipe file descriptors for input (position 0), output (position 1) and error (position 2)

    char* redirect_out;   //file name for redirect command's output to file
    char* redirect_in;    //file name for redirect command's input from file
    char* redirect_error; //file name for redirect command's error to file
};

//Indicates if shell process received SIGINT and should stop executing subsequent commands
int received_interrupt = FALSE;
//Stores the default SIGINT handler. Used by child processes to enable default behavior 
//(kill process) when receiving the SIGINT. 
__sighandler_t default_handler = NULL;

//function check_file_exists
//
// Utility function to check redirect from files
// parameter: (char*) filename - file to be checked
// return TRUE (1) or FALSE (0)
int check_file_exists(const char *filename);

//function build_command
//
// Populates the struct command c parameter, setting list of command arguments and redirects.
// (Command's pipes are configured later at start_command method).
// Parameters:  command* c - command struct which will get configured
//              char* line - command line executed in shell
//              start_index - command start index in command line string
//              end_index   - command end index in command line string
void build_command(struct command *c, const char* line, int start_index, int end_index);

// command_append_arg(c, word)
//    Add `word` as an argument to command `c`. This increments `c->argc`
//    and augments `c->argv`.

static void command_append_arg(command* c, char* word) {
    c->argv = (char**) realloc(c->argv, sizeof(char*) * (c->argc + 2));
    c->argv[c->argc] = word;
    c->argv[c->argc + 1] = NULL;
    ++c->argc;
}

//function interrupt_handler
//
// Overrides default SIGINT behavior by setting interrupt flag = TRUE
// and printing prompt for shell process. Child process will not use this handler.
void interrupt_handler() 
{
    received_interrupt = TRUE;
    printf("\nsh61[%d]$ ", getpid());
    fflush(stdout);
}

// COMMAND EVALUATION

// start_command(c, pgid)
//    Start the single command indicated by `c`. Sets `c->pid` to the child
//    process running the command, and returns `c->pid`.
//
//    PART 1: Fork a child process and run the command using `execvp`.
//    PART 5: Set up a pipeline if appropriate. This may require creating a
//       new pipe (`pipe` system call), and/or replacing the child process's
//       standard input/output with parts of the pipe (`dup2` and `close`).
//       Draw pictures!
//    PART 7: Handle redirections.
//    PART 8: The child process should be in the process group `pgid`, or
//       its own process group (if `pgid == 0`). To avoid race conditions,
//       this will require TWO calls to `setpgid`.

pid_t start_command(command* c, pid_t pgid) {
    (void) pgid;
    int ret_pid;

    // Create a pipe to be used by child processes to communicate
    // Has to be created by parent process to be shared among children
    if (c->pipe_out){
        assert(pipe(c->pipefd) == 0);
    }

    ret_pid = fork();
    switch (ret_pid){
        /* I am the child */
        case 0:

            // Restore the standard SIGINT behavior 
            // (kill command if CTRL+C is pressed)
            signal(SIGINT, default_handler);

            // if command pipe_out indicator == TRUE, set STDOUT to pipe
            if (c->pipe_out){
                dup2(c->pipefd[PIPE_WRITE], STDOUT_FILENO);
                
                close(c->pipefd[PIPE_READ]);
                close(c->pipefd[PIPE_WRITE]);
            }
            // Else, if redirect_out is not NULL, write the commands STDOUT to file 
            else if (c->redirect_out){

                // Checks if directory exist
                if (opendir(dirname(c->redirect_out)) == 0){
                    printf("No such file or directory ");
                    exit(1);
                }

                // Open and create / truncate the file, and set STDIO to its descriptor
                int out_fd = open(c ->redirect_out, O_CREAT | O_WRONLY | O_TRUNC, 0664 );
                dup2(out_fd, STDOUT_FILENO);
                close(out_fd);
            }
            
            // if redirect from file is not NULL, set STDIN from input file descriptor
            if (c->redirect_in){

                // Checks if input file exist / is accessible
                if (!check_file_exists(c->redirect_in)){
                    printf("No such file or directory ");
                    exit (1);
                }

                int in_fd = open(c ->redirect_in, O_CREAT | O_RDONLY );
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            } 
            // Else, if command pipe_in indicator == TRUE, set STDIN to pipe  
            else if (c->pipe_in){
                struct command* prev = c-1;

                dup2(prev->pipefd[PIPE_READ], STDIN_FILENO);
                close(prev->pipefd[PIPE_WRITE]);
                close(prev->pipefd[PIPE_READ]);
            }

            // If error redirect parameter is set, pipes command STDERR to file
            if (c->redirect_error){
                // Checks if output file exist
                int error_fd = open(c ->redirect_error, O_CREAT | O_WRONLY | O_TRUNC, 0664 );
                dup2(error_fd, STDERR_FILENO);
                close(error_fd);
            }

            // Executes the command!
            if (execvp(c->argv[0], c->argv) == -1) {
                printf("Something bad happened: %s\n", strerror(errno));
            }
            break;
        default:
            /* I am the parent */

            // If pipes were used on previous command, close them on parent for good hygiene
            if (c->pipe_in){
                struct command* prev = c-1;
                close(prev->pipefd[PIPE_READ]);
                close(prev->pipefd[PIPE_WRITE]);
            }
            break;
    }
    c->pid = ret_pid;
            
    return c->pid;
}

// run_list(c)
//    Run the command list starting at `c`.
//
//    PART 1: Start the single command `c` with `start_command`,
//        and wait for it to finish using `waitpid`.
//    The remaining parts may require that you change `struct command`
//    (e.g., to track whether a command is in the background)
//    and write code in run_list (or in helper functions!).
//    PART 2: Treat background commands differently.
//    PART 3: Introduce a loop to run all commands in the list.
//    PART 4: Change the loop to handle conditionals.
//    PART 5: Change the loop to handle pipelines. Start all processes in
//       the pipeline in parallel. The status of a pipeline is the status of
//       its LAST command.
//    PART 8: - Choose a process group for each pipeline.
//       - Call `set_foreground(pgid)` before waiting for the pipeline.
//       - Call `set_foreground(0)` once the pipeline is complete.
//       - Cancel the list when you detect interruption.

void run_list(command* commandList) {
    int ret_pid;
    int exit_status = !EXIT_SUCCESS;   /* command's exit status */
    int is_background = FALSE;         /* indicates if expression should execute in background. An "expression" is a list of commands separated by logical operators / pipes / redirection */
    int expr_pid;                      /* expression process id */

    // if expression (list of commands) ended with '&', the shell will fork and continue executing,
    // while each command in the expression is executed in sequence (with waitpid).
    if (commandList->is_background){
        is_background = TRUE;
        expr_pid = fork();
        //If parent process, return
        if (expr_pid < 0){
            printf("fork() failed!\n");
            exit(1);
        }
        //if parent, return
        else if (expr_pid){
            return;
        }
    }

    // For each command in the expression, and until SIGINT is not received... 
    for (int i = 0; i < MAX_COMMANDS_PER_LINE && !received_interrupt; i++){
        struct command* c = &commandList[i];
        
        // If command is empty, stop executing 
        if (!c->argc)
            break;

        // Is "cd" command?
        if (strcmp(*c->argv, "cd") == 0){
            exit_status = EXIT_SUCCESS;

            // Checks if directory exists 
            if (opendir(c->argv[1]) != 0){
                // Changes the current directory
                if (chdir (c->argv[1]) != 0){
                    printf("Error changing directory\n");
                    exit_status = -1;
                }
            }
            else{
                exit_status = -1;
            }
        }
        else{

            // Forks and executes the command
            ret_pid = start_command(c, 0);

            // If I am the expression process and the current command's output is not piped,            
            if (ret_pid && !c->pipe_out){
                /* I am parent */

                // Close previous commands pipes
                for (int j = i-1; j >= 0; j--){
                    if (commandList[j].pipefd[0]){
                        close(commandList[j].pipefd[PIPE_READ]);
                        close(commandList[j].pipefd[PIPE_WRITE]);
                    }
                }
                // Waits for all expression's commands to finish. 
                // Note that if commands should run in background (marked with '&' token), the expression process
                // was forked and "detached" from shell. But the expression still need to wait on its list of commands
                // in order to pipe in / pipe out results, perform logical checks between commands, redirects, etc.
                if (waitpid(ret_pid, &exit_status, 0) != ret_pid)
                    printf ("Unable to get response from command. Exit status: %d\n", exit_status);
            }
        }
        
        // Checks if previous command exit status and the logical operator on the right side will lead to skipping the next command
        if (exit_status == EXIT_SUCCESS && c->logical_op == LOGICAL_OR){
            i++; // skip next command
        } else if (!(exit_status == EXIT_SUCCESS) && c->logical_op == LOGICAL_AND){
            i++; // skip next command
        }
    }

    // If expression was set to run in background, it's a separate process and
    // needs to be killed.
    if (is_background)
        pthread_exit(0);
}


// eval_line(c)
//    Parse the command list in `s` and run it via `run_list`.
void eval_line(const char* s) {
    int length = strlen(s);
    int start = 0;
    
    int double_quotes = 0;   /* Double quote count. Checks for string literals on commands */
    int single_quotes = 0;   /* Single quote count. Checks for string literals on commands */

    int c_index = 0;             /* Index used by command list */
    int prev_pipe_out = FALSE;   /* Indicates if previous command writes to pipe */
    received_interrupt = FALSE;  /* Indicates if CTRL+C was used and command line should stop executing */

    // Allocates list of commands
    struct command * commands = calloc (MAX_COMMANDS_PER_LINE, sizeof(struct command));  

    // For each input character and while it was not interrupted... 
    for (int i = 0; i < length && !received_interrupt; i++) {  
    
        // If the command includes a string literal... 
        if (s[i] == '"'){
            double_quotes++;
            if (double_quotes == 2)
                double_quotes = 0;
        } else if (s[i] == '\''){
            single_quotes++;
            if (single_quotes == 2)
                single_quotes = 0;
        }

        // ..  skips until end quote
        if (double_quotes != 1 && single_quotes != 1){
            
            // If found logic operator, add command to expression (list of commands)
            if (i < length - 1 &&
                ((s[i] == '&' && s[i+1] == '&') ||
                 (s[i] == '|' && s[i+1] == '|'))){

                struct command* c = &commands[c_index++];
                build_command(c, s, start, i);

                // Sets logical operand flag to LOGICAL_AND / OR
                if (s[i] == '&')
                    c->logical_op = LOGICAL_AND;
                else
                    c->logical_op = LOGICAL_OR;

                // If previous command's output is piped, sets current command input pipe to TRUE
                if (prev_pipe_out){
                    c->pipe_in = TRUE;
                    prev_pipe_out = FALSE;
                }

                i=i+2;
                start = i;

            // If found pipe operator, add command to expression (list of commands)
            } else if (s[i] == '|'){

                struct command* c = &commands[c_index++];
                build_command(c, s, start, i);

                // Pipe token was found on command right's site,
                // indicating pipe output to next command
                c->pipe_out = TRUE;
                
                // If previous command's output is piped, sets current command input pipe to TRUE
                if (prev_pipe_out)
                    c->pipe_in = TRUE;
                
                prev_pipe_out = TRUE;
                i++;
                start = i;
                
            //If End of Expression, run commands list
            } else if (s[i] == ';' || s[i] == '&'){

                // Set backgound process indicator
                if(s[i] == '&'){
                    commands->is_background = TRUE;
                }

                struct command* c = &commands[c_index++];
                build_command(c, s, start, i);

                // Same as above: If previous command's output is piped, sets current command input pipe to TRUE
                if (prev_pipe_out){
                    c->pipe_in = TRUE;
                    prev_pipe_out = FALSE;
                }

                // Run command list
                run_list(commands);

                // clears the command list and reset index
                memset(commands, 0, MAX_COMMANDS_PER_LINE * sizeof(struct command));
                c_index = 0;

                i++;
                start = i;
            }
        }
        waitpid(-1, 0, WNOHANG);
    }

    // At the end of command line (iow, semi-colon or pipe tokens not found)...
    if (start < length - 1  && !received_interrupt){

        struct command* c = &commands[c_index++];
        build_command(c, s, start, length);

        if (prev_pipe_out){
            c->pipe_in = TRUE;
            prev_pipe_out = FALSE;
        }

        // build and execute list of remaining commands
        run_list(commands);
    }

    //deallocates list of commands
    free(commands);
}

//function build_commands
//
// Set the redirection flags, and parse commands attribute list using 'parse_shell_token'
// Parameters:  struct command *c - Command where flags are being set
//              line - input string typed at shell
//              start and end_index -  delimiter positions at command line 
void build_command(struct command *c, const char* line, int start_index, int end_index){
    int type;
    char* token;

    int prev_type = -1;
    char* prev_token = NULL;

    // allocates and copies command string into command struct
    char * commandStr = calloc(end_index - start_index + 1, sizeof(char));
    memcpy(commandStr, line + start_index, end_index - start_index);

    // build the command
    while ((commandStr = (char*) parse_shell_token(commandStr, &type, &token)) != NULL){

        //  If redirection token is found, sets the filename as redirection parameter
        // NOTE: diferent from pipes, I am referencing the filename string instead of file descriptor
        // for redirects, because of file / directory checks to be performed at 'start_command'.
        if (prev_type == TOKEN_REDIRECTION){
            if (strcmp(">", prev_token) == 0)
                c->redirect_out = strdup(token);
            else if (strcmp("<", prev_token) == 0)
                c->redirect_in = strdup(token);
            else if (strcmp("2>", prev_token) == 0)
                c->redirect_error = strdup(token);

            // removes the last command argument (which was the redirect)
            c->argv[c->argc-1] = NULL;
            c->argc--;
            
            // Resets the prev_token flag
            prev_token = NULL;
            prev_type = -1;
            continue;
        }

        command_append_arg(c, token);
        // Saves the previous token (just in case it's a redirect filename)
        prev_token = token;
        prev_type = type;
    }
    free(commandStr);
}

// function check_file_exists
//
// Utility function to check if file exists
int check_file_exists(const char *filename) {
    struct stat st;
    int result = stat(filename, &st);
    return result == 0;
}

int main(int argc, char* argv[]) {
    FILE* command_file = stdin;
    int quiet = 0;

    // Check for '-q' option: be quiet (print no prompts)
    if (argc > 1 && strcmp(argv[1], "-q") == 0) {
        quiet = 1;
        --argc, ++argv;
    }

    // Check for filename option: read commands from file
    if (argc > 1) {
        command_file = fopen(argv[1], "rb");
        if (!command_file) {
            perror(argv[1]);
            exit(1);
        }
    }

    // Signal handler for ctrl+c
    default_handler = signal(SIGINT,interrupt_handler); 

    // - Put the shell into the foreground
    // - Ignore the SIGTTOU signal, which is sent when the shell is put back
    //   into the foreground
    set_foreground(0);
    handle_signal(SIGTTOU, SIG_IGN);

    char buf[BUFSIZ];
    int bufpos = 0;
    int needprompt = 1;

    while (!feof(command_file)) {
        // Print the prompt at the beginning of the line
        if (needprompt && !quiet) {
            printf("sh61[%d]$ ", getpid());
            fflush(stdout);
            needprompt = 0;
        }

        // Read a string, checking for error or EOF
        if (fgets(&buf[bufpos], BUFSIZ - bufpos, command_file) == NULL) {
            if (ferror(command_file) && errno == EINTR) {
                // ignore EINTR errors
                clearerr(command_file);
                buf[bufpos] = 0;
            } else {
                if (ferror(command_file))
                    perror("sh61");
                break;
            }
        }

        // If a complete command line has been provided, run it
        bufpos = strlen(buf);
        if (bufpos == BUFSIZ - 1 || (bufpos > 0 && buf[bufpos - 1] == '\n')) {
            eval_line(buf);
            bufpos = 0;
            needprompt = 1;
        }

        // Handle zombie processes and/or interrupt requests
        waitpid(-1, 0, WNOHANG);
    }

    return 0;
}