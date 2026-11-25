# Lesson 00 — Setup & mental model

> [[README|Course index]] · [[01-the-read-loop|Lesson 01]] →

## Direct answer

A **shell** is just a program that reads a line you typed, finds the program you asked for, runs it, waits for it to finish, and then asks for the next line. That is all. Everything else (pipes, redirections, `cd`) is built on top of this loop.

By the end of this lesson you will have a working compiler and a clear picture of the three words you will use all course long: **program**, **process**, **system call**.

---

## Running example

Throughout the course we follow one command:

```
ls -l /tmp
```

It looks trivial, but typing it triggers almost everything a shell does.

---

## 1. Three words you must know

### ① Program
A **file on disk** containing instructions. `ls` is a program: it lives at `/bin/ls` (or `/usr/bin/ls`). A program that is not running does nothing.

### ② Process
A **running instance** of a program. It has a number (the **PID**), its own memory, and a current directory.

```
 /bin/ls   (program, on disk, inactive)
    │
    │  the system runs it
    ▼
 process PID 4242   (alive, has memory, a current directory, open files...)
```

The same program can run many times at once: that gives many processes, each with its own PID.

### ③ System call
Your C code cannot do everything by itself: it cannot create a process, read a file or change directory without asking the **kernel** (the core of the operating system). The request is called a **system call**.

```
 your code ──► library function ──► system call ──► kernel
 printf()        write()
```

`fork`, `execvp`, `waitpid`, `chdir`, `pipe`, `dup2`... are the system calls (or thin wrappers around them) you will use in this course.

💡 Rule of thumb: if the manual page is in section 2 (`man 2 fork`), it is a system call.

---

## 2. What happens when you type `ls -l /tmp`

```
 You type:  ls -l /tmp ⏎
                │
                ▼
        ┌───────────────┐
        │     shell     │  1. reads the line
        │  (a process)  │  2. splits it into words
        └───────┬───────┘  3. creates a new process
                │          4. turns that process into `ls`
                │          5. waits for it
                ▼
        ┌───────────────┐
        │  ls (process) │  prints the directory content
        └───────┬───────┘
                │ finishes with an exit status
                ▼
        shell shows the prompt again
```

You will build these five steps, one lesson at a time:

| Step | Lesson |
|---|---|
| Read the line | [[01-the-read-loop\|01]] |
| Split into words | [[02-splitting-a-line\|02]] |
| Create a process, replace it, wait | [[03-running-a-program\|03]] |

---

## 3. Why the shell is built this way

| Question | Answer |
|---|---|
| Why not run `ls` inside the shell process itself? | The shell would **be replaced** by `ls` and disappear when it ends. |
| So how does the shell survive? | It creates a **copy of itself** first, and only the copy becomes `ls`. |
| Why wait for the copy? | Otherwise the prompt would appear before `ls` finishes printing. |

You will see that creating a copy and replacing it are **two separate operations**. That design is one of the most elegant ideas in Unix, and it is the subject of [[03-running-a-program|lesson 03]].

---

## 4. Set up your environment

### Windows
1. Open PowerShell **as administrator** and run `wsl --install`.
2. Restart, then open the Ubuntu app and create a user.

### Linux / macOS
You already have a terminal.

### Install a compiler (Ubuntu / WSL)

```
sudo apt update
sudo apt install build-essential
gcc --version
```

⚠️ If `gcc --version` prints "command not found", the installation did not finish. Redo the previous step before going further.

---

## 5. Exercise: compile your first program

Create a file `hello.c` that prints one line of your choice, then:

```
gcc -Wall -Wextra -Werror hello.c -o hello
./hello
```

What the flags do:

| Flag | Meaning |
|---|---|
| `-Wall -Wextra` | enable most warnings |
| `-Werror` | treat every warning as an error |
| `-o hello` | name the output file `hello` |

💡 Always compile with these flags. They catch many bugs before you run anything.

### Explore (no code to write)

Run these commands in your terminal and write down what you observe:

```
echo $$          # PID of your current shell
ps               # list your processes
which ls         # where the `ls` program lives
man 2 fork       # read only the first paragraph
```

---

## Check your understanding

1. In your own words, what is the difference between a program and a process?
2. Why can a C program not create a process by itself, without the kernel?
3. If the shell turned itself into `ls`, what would happen when `ls` finishes?
4. What does `echo $$` show, and why does it change every time you open a new terminal?

---

## What's next

In **[[01-the-read-loop|Lesson 01]]** you write the first piece of your shell: a loop that shows a prompt, reads a line and prints it back.
