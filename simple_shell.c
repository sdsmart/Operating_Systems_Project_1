// Includes
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>


// Function Prototypes
void fork_and_execute(char** cmd, int cmd_length, int contains_pipe);
void execute_pipe(char** cmd, int num_tokens);

/*
**************************************
** This program simulates a basic   **
** unix shell by creating child     **   
** processes to execute commands    **
** contained in a given input file. **
**************************************
*/
int main(int argc, char* argv[]) {

    // checking correct command-line usage
    if (argc != 2) {

        printf("Usage: %s file-name\n", argv[0]);

        return 0;

    } else {

        // I/O and parsing variables
        int fd;
        char* buffer;
        char* token;
        char* token_array[10];
        int num_tokens = 0;
        int contains_pipe = 0;

        // opening the input file.
        fd = open(argv[1], O_RDONLY);

        // initializing I/O and parsing variables
        buffer = malloc(1);
        buffer[0] = '\0';
        token = malloc(10);
        token[0] = '\0';
        
        // reading from the input file one byte at a time
        while (read(fd, buffer, 1) == 1) {

            if (buffer[0] == ' ' || buffer[0] == '\n') {

                // allocating memory for token array element and copying
                // the new token inside that element
                token_array[num_tokens] = malloc(strlen(token) + 1);
                strcpy(token_array[num_tokens], token);
                num_tokens++;

                if (buffer[0] == '\n') {

                    // newline has been found therefore the token array
                    // can be completed by appending null to the next element
                    token_array[num_tokens] = NULL;
                    num_tokens++;

                    // forking and executing the commands in the token_array
                    fork_and_execute(token_array, num_tokens, contains_pipe);
                    contains_pipe = 0;

                    // freeing the allocated memory in the token array
		    int i = 0;
                    for (i = 0; i < (num_tokens - 1); i++) {
                        free(token_array[i]);
                    }

                    // resetting the number of tokens contained in the token array
                    num_tokens = 0;

                }

                // re-initializing the token string
                token[0] = '\0';

            } else {
                // appending the single byte read from the file to the token string
                strcat(token, buffer);
            }

            // check if the input command contains a pipe
            if (buffer[0] == '|') {
                contains_pipe = 1;
            }
        }

        // freeing the allocated memory in the token and buffer strings
        free(token);
        free(buffer);

        // exiting the program
        return 0;
   }
}

void fork_and_execute(char** cmd, int num_tokens, int contains_pipe) {

    // variables
    pid_t pid;
    int status;
    const int FORK_FAILURE = -1;
    const int CHILD_PROCESS = 0;

    // forking a new child process
    pid = fork();

    // fork failed
    if (pid == FORK_FAILURE) {
        printf("fork failed\n");
        exit(-1);
    // child process
    } else if (pid == CHILD_PROCESS) {

        // cmd contains pipe so we must spawn another process
        if (contains_pipe) {
            // execute the pipe
            execute_pipe(cmd, num_tokens);
        } else {
            // executing command
            execvp(cmd[0], cmd);

            // if execvp returns then an error occurred
            printf("exec failed: %s\n", strerror(errno));
            exit(0);
        }
    // parent process
    } else {
        // waiting on the child to complete execution
        if (wait(&status) != pid) {
            printf("a signal has interrupted the process\n");
        }
    }                
}

void execute_pipe(char** cmd, int num_tokens) {
    // fork constants
    const int FORK_FAILURE = -1;
    const int CHILD_PROCESS = 0;

    // status variable
    int status;

    // parsing input to split the token array into two separate
    // token arrays (one for each side of the pipe)
    int cmd2_start_index = 0;
    char* cmd2[10];
    int i = 0;
    for (i = 0; i < num_tokens; i++) {
        if (cmd[i][0] == '|') {
            cmd[i] = NULL;
            cmd2_start_index = i + 1;
            break;
        }
    }
    int k = 0;
    int j = cmd2_start_index;
    for (k = 0; k < (num_tokens - cmd2_start_index); k++) {
        cmd2[k] = cmd[j];
        j++;
    }

    // setting up the pipe            
    int fds[2];
    pipe(fds);

    // forking a new child process
    pid_t pid2 = fork();

    // fork failed
    if (pid2 == FORK_FAILURE) {
        printf("fork failed\n");
        exit(-1);
    // child process
    } else if (pid2 == CHILD_PROCESS) {
        // duping output to piped file
        dup2(fds[1], 1);
        close(fds[0]);
        close(fds[1]);

        // executing first command
        execvp(cmd[0], cmd);
    // parent process
    } else {
        // waiting on the child to complete execution
        if (wait(&status) != pid2) {
            printf("a signal has interrupted the process\n");
        }

        // duping input to the piped file
        dup2(fds[0], 0);
        close(fds[0]);
        close(fds[1]);

        // executing second command
        execvp(cmd2[0], cmd2);
    }
}
