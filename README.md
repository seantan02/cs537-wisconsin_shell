
# WSH - A Simple C Shell

WSH (WShell) is a simple Unix-like shell implemented in C. It supports several built-in commands as well as external command execution via `fork()` and `execv()`. This project demonstrates core concepts of shell creation, process management, and command execution in C.

## Features

The shell supports the following **built-in commands**:

1. **`ls`**:  
   Lists files in the current directory in the format of `LANG=C ls -1`.  
   Example:  
   ```sh
   wsh> ls
   file1.txt
   file2.txt
   ```
   
2. **`exit`**:  
   Exits the shell.  
   Example:  
   ```sh
   wsh> exit
   ```

3. **`cd`**:  
   Changes the current working directory. Same behavior as the `cd` command in Linux.  
   Example:  
   ```sh
   wsh> cd /path/to/directory
   ```

4. **`local`**:  
   Sets shell variables in the format `local VAR=VALUE`. These are only available in the shell session and are not exported to the environment.  
   Example:  
   ```sh
   wsh> local MYVAR=hello
   ```

5. **`history`**:  
   Keeps track of all the commands executed in the shell session. You can list the history or re-execute a command from history by typing `history <N>`, where `<N>` is the command number from the list.  
   Example:  
   ```sh
   wsh> history
   1 ls
   2 cd ..
   3 local MYVAR=hello
   4 history
   wsh> history 2
   # Re-executes `cd ..`
   ```

6. **`export`**:  
   Exports a shell variable to the environment, making it available for child processes.  
   Example:  
   ```sh
   wsh> export MYVAR
   ```

7. **`vars`**:  
   Lists all the shell variables currently set.  
   Example:  
   ```sh
   wsh> vars
   MYVAR=hello
   ```

8. **`other commands`**:  
   Any command that is not a built-in command will be executed using `fork()` and `execv()`.  
   Example:  
   ```sh
   wsh> /bin/echo "Hello, world!"
   ```

## Prerequisites

To compile and run the shell, you will need:

- GCC compiler
- POSIX-compliant environment (Linux/Unix-based OS)

The shell requires **C standard gnu18**.

## Building the Shell

To build the `wsh` executable, follow these steps:

1. Clone or download the project to your local machine.
2. Open a terminal and navigate to the project directory.
3. Run the following command to compile the shell:

   ```sh
   gcc -std=gnu18 -o wsh shell.c
   ```

   Replace `shell.c` with the name of your C file that contains the shell implementation.

## Running the Shell

Once compiled, you can start the shell by running:

```sh
./wsh
```

## Usage

The shell will display a prompt where you can type commands. It supports both the built-in commands and any external program invocation.

Example:

```sh
wsh> ls
wsh> local VAR=value
wsh> history
wsh> export VAR
wsh> /bin/echo $VAR
```

### Built-in Command Summary:

| Command          | Description                                                   |
|------------------|---------------------------------------------------------------|
| `ls`             | Lists files in the current directory (like `LANG=C ls -1`).    |
| `exit`           | Exits the shell.                                               |
| `cd <dir>`       | Changes the current working directory.                         |
| `local VAR=VAL`  | Sets a shell variable.                                         |
| `history`        | Lists command history, re-execute with `history <N>`.          |
| `export VAR`     | Exports a shell variable to the environment.                   |
| `vars`           | Lists all shell variables.                                     |

Other commands are executed using the `fork()` and `execv()` approach, so external commands can be run from the shell just like in a typical Unix/Linux shell.

## License

This project is licensed under the MIT License - see the LICENSE file for details.
