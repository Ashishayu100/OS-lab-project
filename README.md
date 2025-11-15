# C-Shell: A Custom Linux Command Line Interpreter

**A lightweight, functional Operating System shell built from scratch in C.**

This project was developed as part of the Operating Systems curriculum at IIT Patna. It demonstrates a deep understanding of process creation, management, system calls, and inter-process communication (IPC) in a UNIX environment.

---

## 🚀 Features

### 1. Command Execution
- parses user input and executes standard Linux commands (e.g., `ls`, `pwd`, `echo`, `grep`).
- Uses the `fork()` and `execvp()` model for process creation.

### 2. I/O Redirection
- **Output Redirection (`>`)**: Captures the output of a command and saves it to a file.
  - *Example:* `ls -l > file_list.txt`
- **Input Redirection (`<`)**: Feeds the contents of a file as input to a command.
  - *Example:* `sort < unsorted_names.txt`

### 3. Piping (`|`)
- Supports chaining two commands together using a pipe. The output of the first command becomes the input of the second.
- *Example:* `ls -l | grep .c`

### 4. Background Processes (`&`)
- Allows commands to run in the background without blocking the shell, enabling asynchronous execution.
- *Example:* `sleep 10 &`

### 5. Built-in Commands
- **`cd`**: Changes the current working directory (utilizing `chdir()`).
- **`exit`**: Terminate the shell session.

---

## 🛠️ Technical Implementation & System Calls

This shell interacts directly with the OS kernel using the following system calls:

| Category | System Call | Usage in Project |
|----------|-------------|------------------|
| **Process Control** | `fork()` | Creates a child process to run commands. |
| | `execvp()` | Replaces the current process image with a new command. |
| | `waitpid()` | Makes the parent (shell) wait for the child to finish. |
| **File I/O** | `open()` | Opens files for redirection. |
| | `dup2()` | Duplicates file descriptors to redirect STDIN/STDOUT. |
| | `close()` | Closes unused file descriptors to prevent leaks. |
| **IPC** | `pipe()` | Creates a unidirectional data channel between processes. |
| **Directory** | `chdir()` | Changes the current working directory of the shell. |

---

## 💻 How to Build and Run

### Prerequisites
- GCC Compiler (`sudo apt install build-essential`)
- Linux Environment (Ubuntu, WSL, etc.)

### Installation
1. **Clone the repository:**
   ```bash
   git clone [https://github.com/your-username/c-shell.git](https://github.com/your-username/c-shell.git)
   cd c-shell