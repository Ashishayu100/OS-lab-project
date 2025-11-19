C-Shell: Advanced Linux Shell (Group 5)

A feature-rich, custom Operating System shell built from scratch in C.

This project was developed by Group 5 as part of the Operating Systems curriculum at IIT Patna. It demonstrates advanced concepts of process management, inter-process communication (IPC), signal handling, and user interface design in a UNIX environment.

(The shell features a dynamic, colored command prompt)

🚀 Key Features

1. 🎨 User Experience (UX)

Dynamic Prompt: Displays the current user and working directory in color (Green/Blue) for a professional look.

Format: [user: /path/to/dir] >

Help System: Built-in help command to guide new users.

Command History: Tracks and lists the last 10 commands using history.

2. ⚡ Process Management

Foreground Execution: Runs standard Linux commands (ls, pwd, echo, etc.) using fork() and execvp().

Background Processes (&): Supports asynchronous execution.

Example: sleep 10 & returns the prompt immediately while the process runs.

3. 🔄 I/O Redirection & IPC

Output Redirection (>): Saves command output to files.

Input Redirection (<): Reads file contents as input.

Pipes (|): Connects the output of one command to the input of another.

Example: ls | grep .c

4. 🛡️ System Stability

Signal Handling: intercept SIGINT (Ctrl+C). Unlike a basic shell, C-Shell protects itself from termination when the user presses Ctrl+C to cancel a running command.

Zombie Harvesting: Properly waits for child processes to prevent resource leaks.

5. 🏠 Built-in Commands

cd <dir>: Changes the current directory (using chdir).

history: Displays session command history.

help: Displays the user manual.

exit: Safely cleans up memory and closes the shell.

🛠️ Technical Architecture

The codebase is modularized to handle different OS responsibilities:

Component

System Calls / Functions

Description

Core Parser

getline, strtok

Reads user input and tokenizes arguments.

Process Control

fork, execvp, waitpid

Manages the creation and execution of new processes.

File I/O

open, dup2, close

Manipulates file descriptors for Redirection (> <).

IPC

pipe, dup2

Creates data channels between two child processes.

Signals

signal(SIGINT)

Catches interrupts to keep the shell alive.

Environment

getcwd, getenv, chdir

Manages the shell's state and prompt.

💻 Installation & Usage

Prerequisites

Linux Environment (Ubuntu, WSL, Fedora, etc.)

GCC Compiler

Build Instructions

Clone the repository:

git clone [https://github.com/YOUR-USERNAME/YOUR-REPO-NAME.git](https://github.com/YOUR-USERNAME/YOUR-REPO-NAME.git)


Compile the project:

gcc shell.c -o shell


Run the shell:

./shell


🧪 Test Cases (Demo)

Try these commands to verify the Group Project features:

1. The Interface:

[ashish: /home/os-project] > help
[ashish: /home/os-project] > cd ..
[ashish: /home] > 


2. Background & Signals:

[ashish: /home] > sleep 5 &
[Background PID: 1234]
[ashish: /home] > sleep 10
(Press Ctrl+C) -> Shell ignores it, command stops, new prompt appears.


3. Advanced Logic (Pipes + Redirection):

[ashish: /home] > ls -l | grep .c > c_files.txt
[ashish: /home] > cat c_files.txt


4. History:

[ashish: /home] > history
1: help
2: cd ..
3: sleep 5 &
...


👥 Team Members (Group 5)

Role

Name

Contribution

Member 1

[Your Name]

Core Logic, Parser, Main Loop

Member 2

[Name]

Process Execution, Background Processes

Member 3

[Name]

I/O Redirection (>, <)

Member 4

[Name]

Pipes (`

Member 5

[Name]

UI (Prompt, History), Signal Handling

Department of Mathematics and Computing Indian Institute of Technology (IIT) Patna