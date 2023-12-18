#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_LINE 510 //The max length of the command
#define MAX_ARGS 10 //The max argument allowed
pid_t GLOBAL_PID, background_pid;

// A struct for the environment variable
typedef struct node{
    char name[MAX_LINE]; // The name of the command
    char value[MAX_LINE]; // The value of the command
    struct node *next;
}Node;

// A method to find the node we are looking for
Node *find_node(Node *head, char *name){
    Node *current = head;
    while(current != NULL){
        if (strcmp(current -> name, name) == 0){ // Finding the correct command to use
            return current;
        }
        current = current -> next;
    }
    return NULL;
}

// A method to add a new node to the list
Node *add_node(Node *head, char *name, char *value, int indicator){
    // Checking to see if the name of the command is the same and changing the value
    Node *found = find_node(head, name);
    if(found != NULL && indicator == 1){
        strcpy(found -> value, value);
    }
    // Allocating new memory for the command that need to be saved
    else {
        Node *new_node = malloc(sizeof(Node));
        if (new_node == NULL) {
            printf("ERR\n");
            free(new_node);
            exit(1);
        }
        strcpy(new_node -> name, name);
        if(value != NULL) {
            strcpy(new_node->value, value);
        }
        new_node -> next = NULL;
        if (head == NULL) { // If the new node is the only node we point to it
            head = new_node;
        } else { // If we have more nodes in the list, we place the new node at the end
            Node *current = head;
            while (current -> next != NULL) {
                current = current -> next;
            }
            current -> next = new_node;
        }
    }
    return head;
}

Node *change_value(Node *head, char *name, char *old_value, char *new_value){
    Node *found = find_node(head, name);
    if(found != NULL){
        char *value_change = found -> value;
        char *new_value_change = value_change + 1;
        if (strcmp(new_value_change, old_value) == 0) {
            if (strlen(new_value) == 0) {
                // If the whole value is equal to old_value and new_value is empty, delete the node
                strcpy(found->value, "");
                return head;
            }
            // If the whole value is equal to old_value and new_value is not empty, update the node value
            strcpy(found -> value, new_value);
            return head;
        }
        // Getting the position of the value we want to change
        char *oldPos = strstr(value_change, old_value);
        if (oldPos != NULL){
            // Copy the part of the value before the old_value
            char prefix[MAX_LINE];
            strncpy(prefix, value_change, oldPos - value_change - 1);
            prefix[oldPos - value_change] = '\0';

            // Copy the part of the value after the old_value
            char suffix[MAX_LINE];
            strcpy(suffix, oldPos + strlen(old_value));

            // Combining both
            char newStr[MAX_LINE];
            if (strlen(new_value) == 0) {
                // If new_value is empty, delete the old_value and keep the rest of the string
                strcpy(newStr, prefix);
                strcat(newStr, suffix);
            } else {
                // If new_value is not empty, replace old_value with new_value
                strcpy(newStr, prefix);
                strcat(newStr, new_value);
                strcat(newStr, suffix);
            }
        }
    }
    return head;
}

void signal_handler(int signum){
    if (signum == SIGTSTP) { // Handling the ctrl z
        if (GLOBAL_PID > 0 && GLOBAL_PID != background_pid) { // If there is a process we stop it
            kill(GLOBAL_PID, SIGSTOP);
        } else {
        }
    }
    else if (signum == SIGCHLD) { // Handling the child process
        // Checking the state of the process we should return immediately if the child process has not yet changed state
        waitpid(GLOBAL_PID, NULL, WNOHANG);
    }
}

// A method to print the prompt to the screen
void print_prompt(int command_count, int arg_count){
    char cwd[1024];
    if(getcwd(cwd, sizeof(cwd)) == NULL){ // Checking to see if the buffer is enough
        printf("Error with 'getcwd()' function\n");
        exit(1);
    }
    printf("#cmd:%d|#args:%d @%s> ", command_count, arg_count, cwd);
}

// A method to remove the quotes from the echo command
char* removeQuotationMarks(const char* input, int* wordCount) {
    size_t len = strlen(input);
    char* result = (char*)malloc((len + 1)*sizeof(char)); // Allocating memory for the result string
    if (result == NULL) {
        printf("Memory allocation failed.\n");
        exit(1);
    }
    size_t i, j = 0;
    int count = 0, inQuotes = 0, pass = 0;
    for (i = 0; i < len; i++) {
        if (input[i] == '"') { // Checking if we are in quotes
            inQuotes = !inQuotes;
            pass++;
            if (pass > 1 && pass % 2 == 0){ // Counting the word inside the quotes
                count++;
            }
        } else if (input[i] == ' ' && !inQuotes && pass > 1 && pass % 2 == 0) { // If we are not in quotes we count the word
            count++;
        }

        if (!inQuotes) { // We change the string
            result[j] = input[i];
            j++;
        }
        if (inQuotes) { // Changing the state of inQuotes to need miss chars
            inQuotes = !inQuotes;
        }
    }
    count++; // Adding the last word
    *wordCount = count;
    result[j] = '\0'; // Null terminate the end of the result string
    return result;
}

// A method to remove leading spaces
void removeLeadingSpaces(char* str) {
    size_t len = strlen(str);
    size_t leadingSpaces = strspn(str, " ");

    if (leadingSpaces > 0) { // Adjust the pointer to remove leading spaces
        memmove(str, str + leadingSpaces, len - leadingSpaces + 1);
    }
}

// A method to release the memory for the linked list
void free_list(Node *head){
    Node *current = head;
    while (current != NULL) {
        Node *next = current->next;
        free(current);
        current = next;
    }
    free(current);
}

int main(void) {
    // Directing the signals to the correct location to be handled
    signal(SIGTSTP, signal_handler);
    signal(SIGCHLD, signal_handler);

    char *args[MAX_ARGS + 1]; // Array for the command arguments
    memset(args, 0, sizeof(args)); // Initialize the array to avoid garbage values
    char command[MAX_LINE + 1]; // The command entered by the user
    // The number of executed commands/arguments and an exit counter
    int command_count = 0, arg_count = 0, exit_count = 0, output_file = 0;
    Node *env_vars = NULL; // A linked list for the environment variables
    Node *arguments = NULL; // A linked list for the arguments

    while (1) {
        char *result = NULL;
        int numOfPipes = 0, is_pipe = 0, background = 0, pipe_id = 0, get_var = 0;
        print_prompt(command_count, arg_count);
        fgets(command, MAX_LINE + 1, stdin); // Reading the line of the command
        // Checking if the command has a new line char and if we have reached the end of the input
        if (strchr(command, '\n') == NULL && !feof(stdin)) {
            printf("You've exceeded the maximum limit of %d\n", MAX_LINE);
            int c;
            while ((c = getchar()) != '\n' && c != EOF); // Delete the remaining characters in the input buffer
            continue;
        }

        // Checking if the user has pressed enter three times
        if (strcmp(command, "\n") == 0) {
            exit_count++;
            if (exit_count >= 3) {
                free_list(env_vars); // Freeing the memory
                free_list(arguments);
                arguments = NULL;
                if (result != NULL) {
                    free(result);
                }
                exit(0);
            }
            continue;
        } else {
            exit_count = 0;
        }

        // Splitting the input into multiple commands seperated by ';'
        char *outer_savePtr, *inner_savePtr, *pipe_savePtr;
        char name[MAX_LINE], value[MAX_LINE], arg_name[MAX_LINE];
        char *cmd_token = strtok_r(command, ";", &outer_savePtr);
        for (int i = 0; i < strlen(cmd_token); i++) { // Checking for pipes
            if (cmd_token[i] == '|') {
                numOfPipes++;
                is_pipe = 1;
            }
        }
        pid_t pid_array[numOfPipes + 1]; // An array so save the pid of the pipe childes
        memset(args, 0, sizeof(args));
        if (strstr(cmd_token, "&") != NULL) { // If we want to run a background task we check it here
            background = 1;
            // We remove the '&' from the command line
            char *temp_command = strtok(cmd_token, "&");
            strcpy(cmd_token, temp_command);
        }
        while (cmd_token != NULL) {
            int counter = 0;
            // Getting the name of the function and getting the value of that function
            if (sscanf(cmd_token, "%[^=]=%[^\n]", name, value) == 2) {
                // Checking to see that the user want to set environmental variable and not just print to the screen
                if (strstr(name, "echo") == NULL) {
                    char temp_value[MAX_LINE];
                    char temp_name[MAX_LINE];
                    int j = 0, k = 0;
                    for (int p = 0; p < strlen(name); p++) { // Deleting the spaces from the name if it has any
                        if (name[p] != ' ') {
                            temp_name[k] = name[p];
                            k++;
                        }
                    }
                    for (int p = 0; p < strlen(value); p++) { // Deleting the quotes from the value if it has any
                        if (value[p] != '"') {
                            temp_value[j] = value[p];
                            j++;
                        }
                    }
                    temp_name[k] = '\0'; // Ending the string
                    temp_value[j] = '\0';
                    env_vars = add_node(env_vars, temp_name, temp_value, 1); // Adding the function to the linked list
                    // Skipping to the next command, so we won't count it
                    cmd_token = strtok_r(NULL, ";", &outer_savePtr);
                    continue;
                }
            }

            // Recording the arguments
            else {
                char *pipe_token = strtok_r(cmd_token, "|", &pipe_savePtr); // Splitting the pipes
                removeLeadingSpaces(pipe_token);
                if(strstr(pipe_token, "$") != NULL){ // An indicator for env_vars
                    get_var = 1;
                }
                while (pipe_token != NULL) {
                    // Getting the command at one side and the args at the other
                    if (sscanf(pipe_token, "%[^ ] %[^\n]", arg_name, value) == 2) {
                        int num_of_quotes = 0;
                        for (int j = 0; j < strlen(value); j++) { // Checking to see if the args are in quotes
                            if (value[j] == '"'){
                                num_of_quotes++;
                            }
                        }
                        if (num_of_quotes > 1 && num_of_quotes % 2 == 0){ // Removing quotes
                            result = removeQuotationMarks(value, &counter);
                            arguments = add_node(arguments, arg_name, result, 0);
                        }
                        else{ // If we don't have quotes
                            char *arg_token = strtok_r(pipe_token, " \n", &inner_savePtr); // Getting the next argument
                            while (arg_token != NULL){ // Counting the args
                                counter++;
                                arg_token = strtok_r(NULL, " \n", &inner_savePtr); // Getting the next argument
                            }
                            arguments = add_node(arguments, arg_name, value, 0);
                        }
                    }
                    else{ // If we have a command with just one word we go here
                        char *arg_token = strtok_r(pipe_token, " \n", &inner_savePtr);
                        while (arg_token != NULL){
                            counter++;
                            arguments = add_node(arguments, arg_token, NULL, 0);
                            arg_token = strtok_r(NULL, " \n", &inner_savePtr);
                        }
                    }
                    pipe_token = strtok_r(NULL, "|", &pipe_savePtr); // Getting the next command/pipe
                    if (pipe_token != NULL) {
                        removeLeadingSpaces(pipe_token);
                    }
                }
            }

            if (get_var == 1) {
                char *arg; // A variable to see all the string
                char *var_token, *sign_check, *inner, *outer;
                Node *to_be_changed; // A variable to copy the value too
                arg = (char *) (Node *) find_node(arguments, arg_name)->value; // Getting the string
                if (strstr(arg, "$") != NULL) { // Checking for '$' sign
                    sign_check = strtok_r(arg, " \n", &inner);
                    var_token = strtok_r(sign_check, "$", &outer);
                    while (var_token != NULL && strcmp(var_token, " ") != 0) {
                        to_be_changed = find_node(env_vars, var_token); // Getting the value we want
                        if (to_be_changed != NULL) { // Changing the value
                            change_value(arguments, arg_name, var_token, to_be_changed->value);
                        }
                            // If there is not such variable we delete the unused variable
                        else if (strstr(sign_check, "$") != NULL && to_be_changed == NULL) {
                            change_value(arguments, arg_name, var_token, "");
                        }
                        sign_check = strtok_r(NULL, " \n", &inner);
                        var_token = strtok_r(sign_check, "$", &outer);
                    }
                }
            }

            if (counter > MAX_ARGS) { // If the max argument has been passed we skip the command and go to  the next one
                printf("Too many arguments, max allowed is: %d\n", MAX_ARGS);
                arguments -> value[0] = '\0';
                free_list(arguments);
                arguments = NULL;
                cmd_token = strtok_r(NULL, ";", &outer_savePtr);
                continue;
            }
            if (find_node(arguments, "bg") != NULL) { // If we get 'bg' we make the process run in the background
                kill(GLOBAL_PID, SIGCONT);
                printf("Process has resumed in the background\n");
                arguments -> value[0] = '\0';
                free_list(arguments);
                arguments = NULL;
                cmd_token = strtok_r(NULL, ";", &outer_savePtr);
                continue;
            }
            if (find_node(arguments, "cd") != NULL) { // Not allowing the use of the 'cd' command
                printf("cd not supported\n");
                arguments -> value[0] = '\0';
                free_list(arguments);
                arguments = NULL;
                cmd_token = strtok_r(NULL, ";", &outer_savePtr);
                continue;
            }
            arg_count += counter; // Setting the number of argument

            // Opening the pipes
            int fd[numOfPipes][2];
            for (int k = 0; k < numOfPipes; k++) {
                if (pipe(fd[k]) == -1) {
                    printf("Error opening the pipe\n");
                    free_list(env_vars); // Freeing the memory
                    free_list(arguments);
                    arguments = NULL;
                    free(result);
                    exit(1);
                }
            }

            char *args_token, *outer_ptr;
            while (arguments != NULL) { // Converting the linked list of args into an array
                int u = 0;
                args[u] = arguments->name;
                args_token = strtok_r(arguments->value, " ", &outer_ptr);
                while (args_token != NULL) {
                    u++;
                    if (strcmp(args_token, "") == 0) {
                        args[u] = NULL;
                    } else {
                        args[u] = args_token;
                    }
                    args_token = strtok_r(NULL, " ", &outer_ptr);
                }
                args[u + 1] = NULL;
                arguments = arguments->next;

                int redirect = 0;
                for (int j = 0; j < u; j++) { // Checking to see if we have a redirect command
                    if (strcmp(args[j], ">") == 0) {
                        redirect = 1;
                        output_file = open(args[j + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        // Removing the redirect arg from the array
                        args[j] = NULL;
                        args[j + 1] = NULL;
                        break;
                    }
                }
                // Executing the commands
                GLOBAL_PID = fork();
                command_count++;
                if (GLOBAL_PID < 0) {
                    // Fork failed
                    printf("ERR\n");
                    free_list(env_vars); // Freeing the memory
                    free_list(arguments);
                    arguments = NULL;
                    free(result);
                    exit(1);
                } else if (GLOBAL_PID == 0) {
                    if (numOfPipes > 0) {
                        if (pipe_id == 0) {
                            // First command, redirect STDOUT to the write end of the pipe
                            dup2(fd[pipe_id][1], STDOUT_FILENO);
                            close(fd[pipe_id][0]);
                        } else if (pipe_id == numOfPipes) {
                            // Last command, redirect STDIN to the read end of the previous pipe
                            dup2(fd[pipe_id - 1][0], STDIN_FILENO);
                            close(fd[pipe_id - 1][1]);
                        } else {
                            // Middle command, redirect both STDIN and STDOUT to the appropriate pipes
                            dup2(fd[pipe_id - 1][0], STDIN_FILENO);
                            close(fd[pipe_id - 1][1]);
                            dup2(fd[pipe_id][1], STDOUT_FILENO);
                            close(fd[pipe_id][0]);
                        }
                    }
                    if (redirect == 1){ // Redirect STDOUT to the output file
                        dup2(output_file, STDOUT_FILENO);
                    }
                    if (execvp(args[0], args) < 0) {
                        printf("ERR\n");
                        free_list(env_vars); // Freeing the memory
                        free_list(arguments);
                        arguments = NULL;
                        free(result);
                        exit(1);
                    }
                } else { // Parent process
                    // Saving the pid so what we can wait for them after the piping process is done
                    pid_array[pipe_id] = GLOBAL_PID;
                    if (numOfPipes > 0) {
                        close(fd[pipe_id][1]); // Close the write end of the pipe
                    }
                    if (pipe_id > 0) {
                        close(fd[pipe_id - 1][0]); // Close the read end of the previous pipe
                    }
                    if (output_file > 0){ // Closing the output file
                        close(output_file);
                    }
                    if (pipe_id == numOfPipes) {
                        if (background == 0 && is_pipe == 0) { // Foreground process, wait for child to complete
                            waitpid(GLOBAL_PID, NULL, WUNTRACED);
                            GLOBAL_PID = 0;
                        }
                        else if (is_pipe == 1){ // Waiting for all the pid of the pipes to end
                            for (int j = 0; j <= numOfPipes + 1; j++) {
                                waitpid(pid_array[j], NULL, 0);
                            }
                        }
                        else { // Background process, continue without waiting for child
                            background_pid = GLOBAL_PID;
                        }
                    }
                    if (is_pipe == 1) { // Increasing the pipe number to get the correct one
                        pipe_id++;
                    }
                }
            }
            cmd_token = strtok_r(NULL, ";", &outer_savePtr); // Getting the next command
        }
    }
}