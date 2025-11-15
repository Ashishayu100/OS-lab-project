#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_ARGS 64
#define DELIMITERS " \t\n"

/*
 * Function: parse_line
 * (Unchanged)
 */
void parse_line(char *line, char **args, char **input_file, char **output_file) {
    int i = 0;
    char *token;
    *input_file = NULL;
    *output_file = NULL;

    token = strtok(line, DELIMITERS);
    while (token != NULL) {
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, DELIMITERS);
            if (token == NULL) {
                fprintf(stderr, "Error: No input file specified after <\n");
                break;
            }
            *input_file = token;
        } else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, DELIMITERS);
            if (token == NULL) {
                fprintf(stderr, "Error: No output file specified after >\n");
                break;
            }
            *output_file = token;
        } else {
            args[i] = token;
            i++;
        }
        token = strtok(NULL, DELIMITERS);
    }
    args[i] = NULL;
}

/*
 * Function: execute_command
 * (Unchanged)
 */
void execute_command(char **args, char *input_file, char *output_file,int background) {
    pid_t pid = fork();

    if (pid == 0) {
        // --- Child Process ---
        if (output_file != NULL) {
            int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd_out == -1) { perror("open"); exit(EXIT_FAILURE); }
            if (dup2(fd_out, STDOUT_FILENO) == -1) { perror("dup2"); exit(EXIT_FAILURE); }
            close(fd_out);
        }
        if (input_file != NULL) {
            int fd_in = open(input_file, O_RDONLY);
            if (fd_in == -1) { perror("open"); exit(EXIT_FAILURE); }
            if (dup2(fd_in, STDIN_FILENO) == -1) { perror("dup2"); exit(EXIT_FAILURE); }
            close(fd_in);
        }
        if (execvp(args[0], args) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } else if (pid < 0) {
        perror("fork");
    } else {
        // --- Parent Process Logic Updated ---
        if (background == 1) {
            // If background, print the PID and DO NOT wait
            printf("[Running in background] PID: %d\n", pid);
        } else {
            // Normal mode: Wait for child
            int status;
            waitpid(pid, &status, 0);
        }
    }
}

/*
 * Function: parse_for_pipe
 * (Unchanged)
 */
int parse_for_pipe(char *line, char **cmd1, char **cmd2) {
    char *pipe_char = strchr(line, '|');
    if (pipe_char != NULL) {
        *pipe_char = '\0';
        *cmd1 = line;
        *cmd2 = pipe_char + 1;
        if (strlen(*cmd2) == 0) {
            fprintf(stderr, "Error: Missing command after pipe.\n");
            return -1;
        }
        return 1;
    } else {
        return 0;
    }
}


// --- THIS FUNCTION IS COMPLETELY REWRITTEN ---

/*
 * Function: execute_pipe
 * ----------------------
 * Executes two commands connected by a pipe.
 */
void execute_pipe(char *cmd1, char *cmd2) {
    int pipefd[2]; // Array to hold the two file descriptors for the pipe
    pid_t pid1, pid2;

    char *args1[MAX_ARGS + 1];
    char *args2[MAX_ARGS + 1];
    
    // Parse both commands. We ignore redirection for pipes to keep it simple.
    char *in_file = NULL;
    char *out_file = NULL;
    parse_line(cmd1, args1, &in_file, &out_file);
    parse_line(cmd2, args2, &in_file, &out_file);
    
    if (args1[0] == NULL || args2[0] == NULL) {
        fprintf(stderr, "Error: Invalid null command in pipe.\n");
        return;
    }

    // 1. Create the pipe
    if (pipe(pipefd) < 0) {
        perror("pipe");
        return;
    }

    // 2. Fork the first child (for cmd1)
    pid1 = fork();
    if (pid1 < 0) {
        perror("fork");
        return;
    }

    if (pid1 == 0) {
        // --- Child 1 (cmd1) ---
        
        // We don't need to read from the pipe
        close(pipefd[0]);
        
        // Redirect STDOUT (1) to the pipe's write end (pipefd[1])
        dup2(pipefd[1], STDOUT_FILENO);
        
        // Close the original write end
        close(pipefd[1]);
        
        // Execute cmd1
        if (execvp(args1[0], args1) == -1) {
            perror("execvp cmd1");
            exit(EXIT_FAILURE);
        }
    }

    // 3. Fork the second child (for cmd2)
    pid2 = fork();
    if (pid2 < 0) {
        perror("fork");
        return;
    }

    if (pid2 == 0) {
        // --- Child 2 (cmd2) ---
        
        // We don't need to write to the pipe
        close(pipefd[1]);
        
        // Redirect STDIN (0) from the pipe's read end (pipefd[0])
        dup2(pipefd[0], STDIN_FILENO);
        
        // Close the original read end
        close(pipefd[0]);
        
        // Execute cmd2
        if (execvp(args2[0], args2) == -1) {
            perror("execvp cmd2");
            exit(EXIT_FAILURE);
        }
    }

    // --- Parent Process ---
    // 4. Close both ends of the pipe in the parent
    //    This is CRITICAL. If we don't, Child 2 will never get EOF
    //    and might hang, waiting for more input from the pipe.
    close(pipefd[0]);
    close(pipefd[1]);

    // 5. Wait for both children to finish
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

// Function to handle built-in commands like cd and exit
// Returns 1 if a built-in command was executed, 0 otherwise
int handle_builtin(char **args) {
    // Handle "exit"
    if (strcmp(args[0], "exit") == 0) {
        exit(0);
    }

    // Handle "cd"
    if (strcmp(args[0], "cd") == 0) {
        // If no directory is provided (just "cd"), go to HOME
        if (args[1] == NULL) {
            chdir(getenv("HOME")); 
        } 
        else {
            // Try to change to the specified directory
            if (chdir(args[1]) != 0) {
                perror("cd"); // Print error if folder doesn't exist
            }
        }
        return 1; // We executed a built-in
    }

    return 0; // Not a built-in
}


// --- MAIN FUNCTION IS UNCHANGED FROM PREVIOUS STEP ---

int main() {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    
    char *args[MAX_ARGS + 1];
    char *input_file = NULL;
    char *output_file = NULL;
    
    char *cmd1 = NULL;
    char *cmd2 = NULL;

    while (1) {
        printf("> ");
        fflush(stdout);

        read = getline(&line, &len, stdin);

        if (read == -1) {
            printf("\nGoodbye!\n");
            break;
        }
        if (line[read - 1] == '\n') {
            line[read - 1] = '\0';
        }
        if (strcmp(line, "") == 0) {
            continue;
        }
        if (strcmp(line, "exit") == 0) {
            break;
        }

        // --- NEW LOGIC FLOW ---
        int pipe_found = parse_for_pipe(line, &cmd1, &cmd2);

        if (pipe_found == 1) {
            // --- PIPE LOGIC ---
            execute_pipe(cmd1, cmd2);

        } else if (pipe_found == 0) {
            // --- NO PIPE LOGIC ---
            parse_line(line, args, &input_file, &output_file);
            if (args[0] == NULL) {
                continue;
            }

            // --- NEW: CHECK FOR BUILT-INS ---
            // If handle_builtin returns 1, we successfully ran cd or exit.
            // So we continue to the next loop and skip execute_command.
            if (handle_builtin(args) == 1) {
                continue;
            }

            // --- NEW: CHECK FOR BACKGROUND '&' ---
            int background = 0;
            int i = 0;
            // Find the last argument
            while (args[i] != NULL) {
                i++;
            }
            // Check if the last arg is "&"
            if (i > 0 && strcmp(args[i-1], "&") == 0) {
                background = 1;
                args[i-1] = NULL; // Remove the "&" so execvp doesn't see it
            }

            execute_command(args, input_file, output_file,background);
        
        }
        // if (pipe_found == -1), error already printed
    }

    free(line);
    return 0;
}