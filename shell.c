#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_ARGS 64
#define DELIMITERS " \t\n"
#define HISTORY_SIZE 10

// --- ANSI COLOR CODES (For the fancy prompt) ---
#define COLOR_RESET "\033[0m"
#define COLOR_GREEN "\033[1;32m"
#define COLOR_BLUE  "\033[1;34m"
#define COLOR_CYAN  "\033[1;36m"

// Global History
char *history[HISTORY_SIZE];
int history_count = 0;

/*
 * Function: add_to_history
 * Adds a command to the history buffer.
 */
void add_to_history(char *cmd) {
    if (history_count < HISTORY_SIZE) {
        history[history_count++] = strdup(cmd);
    } else {
        free(history[0]);
        for (int i = 1; i < HISTORY_SIZE; i++) {
            history[i - 1] = history[i];
        }
        history[HISTORY_SIZE - 1] = strdup(cmd);
    }
}

/*
 * Function: print_history
 * Prints stored commands.
 */
void print_history() {
    printf(COLOR_CYAN "\n--- Command History ---\n" COLOR_RESET);
    for (int i = 0; i < history_count; i++) {
        printf("%d: %s\n", i + 1, history[i]);
    }
    printf(COLOR_CYAN "-----------------------\n" COLOR_RESET);
}

/*
 * Function: print_help
 * NEW: Adds bulk and documentation to your project.
 */
void print_help() {
    printf("\n");
    printf(COLOR_CYAN "C-SHELL HELP MANUAL" COLOR_RESET "\n");
    printf("-------------------\n");
    printf("Built by Group 5 for OS Project\n\n");
    printf(COLOR_GREEN "Internal Commands:" COLOR_RESET "\n");
    printf("  cd <dir>    : Change the directory to <dir>.\n");
    printf("  exit        : Quit the shell.\n");
    printf("  history     : Show the last 10 commands used.\n");
    printf("  help        : Show this manual.\n\n");
    printf(COLOR_GREEN "Features:" COLOR_RESET "\n");
    printf("  * Redirection : command > file (output), command < file (input)\n");
    printf("  * Pipes       : command1 | command2\n");
    printf("  * Background  : command &\n");
    printf("-------------------\n");
}

/*
 * Function: handle_sigint
 * Catch Ctrl+C (SIGINT)
 */
void handle_sigint(int sig) {
    printf("\n"); // Move to a new line
    // We don't print the prompt here because the loop will do it
}

/*
 * Function: print_prompt
 * NEW: Prints a professional prompt: user@cwd >
 */
void print_prompt() {
    char cwd[1024];
    char *username = getenv("USER"); // Get current user name
    
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        // Print in format: [USER: DIR] >
        // \033[1;32m is Green, \033[1;34m is Blue
        printf(COLOR_GREEN "[%s" COLOR_RESET ":" COLOR_BLUE "%s]" COLOR_RESET " > ", 
               username ? username : "user", cwd);
    } else {
        printf("> ");
    }
    fflush(stdout);
}

/*
 * Function: parse_line (Standard)
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
            if (token == NULL) break;
            *input_file = token;
        } else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, DELIMITERS);
            if (token == NULL) break;
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
 * Function: handle_builtin (Updated with Help)
 */
int handle_builtin(char **args) {
    if (strcmp(args[0], "exit") == 0) {
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
    if (strcmp(args[0], "history") == 0) {
        print_history();
        return 1;
    }
    if (strcmp(args[0], "help") == 0) {
        print_help();
        return 1;
    }
    return 0;
}

/*
 * Function: execute_command (Standard)
 */
void execute_command(char **args, char *input_file, char *output_file, int background) {
    pid_t pid = fork();

    if (pid == 0) {
        signal(SIGINT, SIG_DFL); // Child handles signals normally

        if (output_file != NULL) {
            int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd_out == -1) { perror("open"); exit(EXIT_FAILURE); }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }
        if (input_file != NULL) {
            int fd_in = open(input_file, O_RDONLY);
            if (fd_in == -1) { perror("open"); exit(EXIT_FAILURE); }
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }
        
        if (execvp(args[0], args) == -1) {
            perror("Command not found");
            exit(EXIT_FAILURE);
        }
    } else if (pid < 0) {
        perror("fork");
    } else {
        if (background == 1) {
            printf("[Background PID: %d]\n", pid);
        } else {
            waitpid(pid, NULL, 0);
        }
    }
}

/*
 * Function: parse_for_pipe (Standard)
 */
int parse_for_pipe(char *line, char **cmd1, char **cmd2) {
    char *pipe_char = strchr(line, '|');
    if (pipe_char != NULL) {
        *pipe_char = '\0';
        *cmd1 = line;
        *cmd2 = pipe_char + 1;
        return 1;
    }
    return 0;
}

/*
 * Function: execute_pipe (Standard)
 */
void execute_pipe(char *cmd1, char *cmd2) {
    int pipefd[2];
    pid_t pid1, pid2;
    char *args1[MAX_ARGS + 1], *args2[MAX_ARGS + 1];
    char *i1 = NULL, *o1 = NULL, *i2 = NULL, *o2 = NULL;

    parse_line(cmd1, args1, &i1, &o1);
    parse_line(cmd2, args2, &i2, &o2);

    if (args1[0] == NULL || args2[0] == NULL) return;
    if (pipe(pipefd) < 0) return;

    pid1 = fork();
    if (pid1 == 0) {
        signal(SIGINT, SIG_DFL);
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        if (execvp(args1[0], args1) == -1) exit(EXIT_FAILURE);
    }

    pid2 = fork();
    if (pid2 == 0) {
        signal(SIGINT, SIG_DFL);
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        if (execvp(args2[0], args2) == -1) exit(EXIT_FAILURE);
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

    signal(SIGINT, handle_sigint);

    while (1) {
        // NEW: Print the fancy prompt
        print_prompt();

        read = getline(&line, &len, stdin);

        if (read == -1) {
            printf("\nGoodbye!\n");
            break;
        }
        if (line[read - 1] == '\n') line[read - 1] = '\0';
        if (strcmp(line, "") == 0) continue;

        add_to_history(line);
        
        int pipe_found = parse_for_pipe(line, &cmd1, &cmd2);

        if (pipe_found == 1) {
            execute_pipe(cmd1, cmd2);
        } else if (pipe_found == 0) {
            parse_line(line, args, &input_file, &output_file);
            if (args[0] == NULL) continue;
            if (handle_builtin(args) == 1) continue;

            int background = 0;
            int i = 0;
            while (args[i] != NULL) i++;
            if (i > 0 && strcmp(args[i-1], "&") == 0) {
                background = 1;
                args[i-1] = NULL;
            }

            execute_command(args, input_file, output_file, background);
        }
    }

    free(line);
    return 0;
}