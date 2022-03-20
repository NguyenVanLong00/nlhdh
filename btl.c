#include <stdio.h>  
#include <string.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h>
#include <sys/wait.h> 
#include <sys/stat.h>
#include <fcntl.h> 
#include <errno.h>

#define MAX_LINE_LENGTH 1024 
#define MAX_LINE 80
#define REDIR_SIZE 2
#define PIPE_SIZE 3
#define MAX_HISTORY_SIZE 128
#define MAX_COMMAND_NAME_LENGTH 128
#define PROMPT_FORMAT "%F %T "
#define PROMPT_MAX_LENGTH 30
#define PIPE_OPT "|"

void remove_end_of_line(char *line) {
    int i = 0;
    while (line[i] != '\n') {
        i++;
    }
    line[i] = '\0';
}

void read_line(char *line) {
    char *ret = fgets(line, MAX_LINE_LENGTH, stdin);
    remove_end_of_line(line);
    if (strcmp(line, "exit") == 0 || ret == NULL || strcmp(line, "quit") == 0) {
        exit(EXIT_SUCCESS); 
    }
}


void parse_command(char *input_string, char **argv, int *wait) {
    int i = 0;
    while (i < MAX_LINE) {
        argv[i] = NULL;
        i++;
    }
    *wait = (input_string[strlen(input_string) - 1] == '&') ? 0 : 1; 
    input_string[strlen(input_string) - 1] = (*wait == 0) ? input_string[strlen(input_string) - 1] = '\0' : input_string[strlen(input_string) - 1];
    i = 0;
    argv[i] = strtok(input_string, " ");

    if (argv[i] == NULL) return;

    while (argv[i] != NULL) {
        i++;
        argv[i] = strtok(NULL, " ");
    }
    argv[i] = NULL;
}

int is_pipe(char **argv) {
    int i = 0;
    while (argv[i] != NULL) {
        if (strcmp(argv[i], PIPE_OPT) == 0) {
            return i; 
        }
        i +=1; 
    }
    return 0; 
}


void parse_pipe(char **argv, char **child01_argv, char **child02_argv, int pipe_index) {
    int i = 0;
    for (i = 0; i < pipe_index; i++) {
        child01_argv[i] = strdup(argv[i]);
    }
    child01_argv[i++] = NULL;

    while (argv[i] != NULL) {
        child02_argv[i - pipe_index - 1] = strdup(argv[i]);
        i++;
    }
    child02_argv[i - pipe_index - 1] = NULL;
}

void exec_child(char **argv) {
    if (execvp(argv[0], argv) < 0) {
        fprintf(stderr, "Error: Failed to execte command.\n");
        exit(EXIT_FAILURE);
    }
}

void exec_child_pipe(char **argv_in, char **argv_out) {
    int fd[2];

    if (pipe(fd) == -1) {
        perror("Error: Pipe failed");
        exit(EXIT_FAILURE);
    }
    if (fork() == 0) {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);
        exec_child(argv_in);
        exit(EXIT_SUCCESS);
    }
    if (fork() == 0) {
        dup2(fd[0], STDIN_FILENO);
        close(fd[1]);
        close(fd[0]);
        exec_child(argv_out);
        exit(EXIT_SUCCESS);
    }
    close(fd[0]);
    close(fd[1]);
    wait(0);
    wait(0);    
}
void set_prev_command(char *history, char *line) {
    strcpy(history, line);
}

char *get_prev_command(char *history) {
    if (history[0] == '\0') {
        fprintf(stderr, "No commands in history\n");
        return NULL;
    }
    return history;
}


void exec_command(char **args, char **redir_argv, int wait, int res) {
    
    if (res == 0) {
        int status;
        pid_t pid = fork();
        if (pid == 0) {
            
            if (res == 0) res = simple_shell_pipe(args);
            if (res == 0) execvp(args[0], args);
            exit(EXIT_SUCCESS);
        } else if (pid < 0) { 
            perror("Error: Error forking");
            exit(EXIT_FAILURE);
        } else { 
            if (wait == 1) {
                waitpid(pid, &status, WUNTRACED); // 
            }
        }
    }
}


int simple_shell_history(char *history, char **redir_args) {
    char *cur_args[MAX_LINE];
    char cur_command[MAX_LINE_LENGTH];
    int t_wait;
    if (history[0] == '\0') {
        fprintf(stderr, "No commands in history\n");
        return 1;
    }
    strcpy(cur_command, history);
    printf("%s\n", cur_command);
    parse_command(cur_command, cur_args, &t_wait);
    int res = 0;
    exec_command(cur_args, redir_args, t_wait, res);
    return res;
}


int simple_shell_pipe(char **args) {
    int pipe_op_index = is_pipe(args);
    if (pipe_op_index != 0) {  
        char *child01_arg[PIPE_SIZE];
        char *child02_arg[PIPE_SIZE];   
        parse_pipe(args, child01_arg, child02_arg, pipe_op_index);
        exec_child_pipe(child01_arg, child02_arg);
        return 1;
    }
    return 0;
}


int main(void) {
    char *args[MAX_LINE];
    char line[MAX_LINE_LENGTH];
  
    char t_line[MAX_LINE_LENGTH];
    char history[MAX_LINE_LENGTH] = "No commands in history";
    char *redir_argv[REDIR_SIZE];
    int wait;

    int res = 0;
    int run = 1;

    while (run) {
        printf("tuanlm>");
        fflush(stdout);
        read_line(line);

        strcpy(t_line, line);
        parse_command(line, args, &wait);
        if (strcmp(args[0], "!!") == 0) {
            res = simple_shell_history(history, redir_argv);
        } else {
            set_prev_command(history, t_line);
            exec_command(args, redir_argv, wait, res);
        }
        res = 0;
    }
    return 0;
}