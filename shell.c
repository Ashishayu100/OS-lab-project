#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h> // NEW: For Signal Handling

#define MAX_ARGS 64
#define DELIMITERS " \t\n"
#define HISTORY_SIZE 10 // NEW: History buffer size

// Global History Array
char *history[HISTORY_SIZE];
int history_count = 0;

/*
 * Function: add_to_history
 * ------------------------
 * Adds a command command to the history buffer.
 */
void add_to_history(char *cmd) {
    if (history_count < HISTORY_SIZE) {
        history[history_count++] = strdup(cmd); // Save a copy
    } else {
        // Shift history to make room (FIFO)
        free(history[0]);
        for (int i = 1; i < HISTORY_SIZE; i++) {
            history[i - 1] = history[i];
        }
        history[HISTORY_SIZE - 1] = strdup(cmd);
    }
}

/*
 * Function: print_history
 * -----------------------
 * Prints the stored commands.
 */
void print_history() {
    for (int i = 0; i < history_count; i++) {
        printf("%d %s\n", i + 1, history[i]);
    }
}

/*
 * Function: handle_sigint
 * -----------------------
 * Catch Ctrl+C (SIGINT) so the shell doesn't crash.
 */
void handle_sigint(int sig) {
    // Print a new line and the prompt
    printf("\n> ");
    fflush(stdout);
}

/*
 * Function: parse_line (Unchanged)
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
 * Function: handle_builtin (Updated for 'history')
 */
int handle_builtin(char **args) {
    if (strcmp(args[0], "exit") == 0) {
        // Free history before exit
        for (int i = 0; i < history_count; i++) free(history[i]);
        exit(0);
    }
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            chdir(getenv("HOME"));
        } else {
            if (chdir(args[1]) != 0) {
                perror("cd");
            }
        }
        return 1;
    }
    // NEW: Check for history command
    if (strcmp(args[0], "history") == 0) {
        print_history();
        return 1;
    }
    return 0;
}

/*
 * Function: execute_command (Updated for Signals)
 */
void execute_command(char **args, char *input_file, char *output_file, int background) {
    pid_t pid = fork();

    if (pid == 0) {
        // --- Child Process ---
        
        // NEW: Reset Signal Handling for the child
        // The shell ignores Ctrl+C, but the child (like sleep) should NOT ignore it.
        // SIG_DFL means "Default Signal Behavior" (i.e., terminate on Ctrl+C)
        signal(SIGINT, SIG_DFL);

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
        // --- Parent Process ---
        if (background == 1) {
            printf("[Background PID: %d]\n", pid);
        } else {
            int status;
            waitpid(pid, &status, 0);
        }
    }
}

/*
 * Function: parse_for_pipe (Unchanged)
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
    }
    return 0;
}

/*
 * Function: execute_pipe (Updated for Signals)
 */
void execute_pipe(char *cmd1, char *cmd2) {
    int pipefd[2];
    pid_t pid1, pid2;
    char *args1[MAX_ARGS + 1];
    char *args2[MAX_ARGS + 1];
    char *in_file = NULL; 
    char *out_file = NULL;

    parse_line(cmd1, args1, &in_file, &out_file);
    parse_line(cmd2, args2, &in_file, &out_file);

    if (args1[0] == NULL || args2[0] == NULL) {
        fprintf(stderr, "Error: Invalid null command in pipe.\n");
        return;
    }

    if (pipe(pipefd) < 0) { perror("pipe"); return; }

    pid1 = fork();
    if (pid1 == 0) {
        // NEW: Reset signal for child
        signal(SIGINT, SIG_DFL);
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        if (execvp(args1[0], args1) == -1) { perror("execvp cmd1"); exit(EXIT_FAILURE); }
    }

    pid2 = fork();
    if (pid2 == 0) {
        // NEW: Reset signal for child
        signal(SIGINT, SIG_DFL);
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        if (execvp(args2[0], args2) == -1) { perror("execvp cmd2"); exit(EXIT_FAILURE); }
    }

    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

int main() {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    char *args[MAX_ARGS + 1];
    char *input_file = NULL;
    char *output_file = NULL;
    char *cmd1 = NULL;
    char *cmd2 = NULL;
    char *line_copy = NULL; // Copy for history

    // NEW: Register Signal Handler
    // This tells the OS: "If user presses Ctrl+C, run handle_sigint() instead of killing me"
    signal(SIGINT, handle_sigint);

    while (1) {
        printf("> ");
        fflush(stdout);

        read = getline(&line, &len, stdin);

        if (read == -1) {
            printf("\nGoodbye!\n");
            break;
        }
        if (line[read - 1] == '\n') line[read - 1] = '\0';
        if (strcmp(line, "") == 0) continue;

        // NEW: Add command to history
        // We must check if it's not just whitespace before adding, but for now, add everything non-empty
        add_to_history(line);
        
        // Keep a copy because strtok destroys the original string
        // (Not strictly necessary for history since we copied it in add_to_history,
        // but good practice if we wanted to use line_copy later)

        // 1. Check for Pipe
        int pipe_found = parse_for_pipe(line, &cmd1, &cmd2);

        if (pipe_found == 1) {
            execute_pipe(cmd1, cmd2);
        } else if (pipe_found == 0) {
            // 2. No Pipe: Parse normally
            parse_line(line, args, &input_file, &output_file);

            if (args[0] == NULL) continue;

            // 3. Check for Built-in (cd, exit, history)
            if (handle_builtin(args) == 1) continue;

            // 4. Check for Background Execution (&)
            int background = 0;
            int i = 0;
            while (args[i] != NULL) i++;
            
            if (i > 0 && strcmp(args[i-1], "&") == 0) {
                background = 1;
                args[i-1] = NULL; // Remove '&' from args
            }

            // 5. Execute
            execute_command(args, input_file, output_file, background);
        }
    }

    free(line);
    return 0;
}