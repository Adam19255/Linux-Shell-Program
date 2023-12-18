# Shell Program with Environment Variables and Multiple Pipes
Authored by Adam Shay

## Description
This shell program supports environment variables, multiple pipes, background processes, output redirections, and more basic commands.
The shell is a program that takes commands from the user and executes them by interacting with the operating system.

## Features
1. **Command Execution:** The program accepts and executes user commands using the execvp system call.
2. **Environment Variables:** Users can set and modify environment variables using the syntax `name=value`. The program maintains a linked list to store the environment variables.
3. **Multiple Pipes:** Users can create multiple pipes to establish communication between multiple processes by redirecting the standard input and output between commands.
3. **Output Redirection:** Users can redirect the output of a command using the `>` symbol.
4. **Background Execution:** Users can run commands in the background by appending an ampersand `&` at the end of the command.
5. **Signal Handling:** The program handles signals such as `SIGTSTP` `(Ctrl-Z)` to stop a running process and `SIGCHLD` to handle child processes.

## How to Use
1. **Prompt:** The program will display a prompt indicating the command count, argument count, and current directory.
2. **Commands:** Enter commands and arguments at the prompt. Use semicolons `;` to separate multiple commands on the same line.
3. **Environment Variables:** Set environment variables using the syntax `name=value`. Use the `echo` command to display environment variables.
4. **Multiple Pipes:** Use the `|` symbol to create multiple pipes and establish communication between commands.
5. **Output Redirection:** Redirect output using `>`. For example: `command > output.txt`.
6. **Background Execution:** Append an ampersand `&` at the end of the command to run it in the background.
7. **Signal Handling:** Press `Ctrl-Z` `(SIGTSTP)` to stop a running process or receive updates on child processes.

## Input Examples
**Execute a command:** `ls -l`
**Set an environment variable:** `VAR=Hello`
**Display an environment variable:** `echo VAR`
**Use multiple pipes:** `ls -l | grep ".txt" | wc -l`
**Redirect output:** `ls -l > output.txt`
**Run a command in the background:** `sleep 10 &`
