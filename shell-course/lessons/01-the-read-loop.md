# Lesson 01 — The read loop

## Direct answer

The first job of a shell is to **show a prompt, wait for a line of text, and repeat until the user quits**. In C, the safest way to read an entire line of unknown length is `getline()`: it allocates and grows the buffer for you.

By the end of this lesson your program will echo every line you type, and stop cleanly on Ctrl-D.

---

## Running example

You type:

```
ls -l /tmp⏎
```

What your program must receive is the string `"ls -l /tmp\n"` (the newline is part of it), in a buffer it owns.

---

## 1. Where does the text come from?

Every process starts with three open channels, called **file descriptors**:

| Number | Name            | C name   | Usually connected to |
| ------ | --------------- | -------- | -------------------- |
| 0      | standard input  | `stdin`  | your keyboard        |
| 1      | standard output | `stdout` | your screen          |
| 2      | standard error  | `stderr` | your screen          |

When you type, the characters wait in `stdin` until you press Enter. Your program then reads them from there.

```
 keyboard ──► terminal ──► stdin ──► your program
                                        │
 screen   ◄── terminal ◄── stdout ◄─────┘
```

---

## 2. Four ways to read a line

### ① `scanf("%s", buf)`
- Stops at the first space, so `ls -l /tmp` would be cut into three reads.
- No protection against a line longer than `buf`.
- ❌ Not suitable.

### ② `gets(buf)`
- Cannot know the size of `buf`: **buffer overflow**.
- Removed from the C standard.
- ❌ Never use it.

### ③ `fgets(buf, size, stdin)`
- Safe, but the **size is fixed**. A longer line is cut, and you must handle the rest yourself.
- Good for small tools, awkward for a shell.

### ④ `getline(&line, &cap, stdin)`
- Allocates the buffer and **grows it** as needed.
- Returns the number of characters read, or `-1` on end of input.
- ✅ Our choice.

| Function      | Any line length | Spaces OK | Safe | Effort |
| ------------- | --------------- | --------- | ---- | ------ |
| `scanf("%s")` | ❌               | ❌         | ❌    | ⭐      |
| `gets`        | ❌               | ✅         | ❌    | ⭐      |
| `fgets`       | ❌ (fixed)       | ✅         | ✅    | ⭐⭐     |
| `getline`     | ✅               | ✅         | ✅    | ⭐      |

### Why `getline`

1. A shell line can have any length, and `getline` handles that.
2. The code is shorter than writing your own growing buffer, so there is less room for bugs.
3. You still learn the ownership rule: **whatever `getline` allocates, you must `free`**.

### When to change

If you must run on a platform without POSIX (so no `getline`), or if you want to read key by key (arrow keys, history), you will need `read()` and your own buffer. That is a much bigger project.

---

## 3. How `getline` works

Its prototype:

```c
ssize_t getline(char **lineptr, size_t *n, FILE *stream);
```

| Parameter  | Meaning                                                                   |
| ---------- | ------------------------------------------------------------------------- |
| `lineptr`  | address of **your** `char *`. `getline` stores the buffer's address there |
| `n`        | address of **your** `size_t`. It holds the buffer's current capacity      |
| `stream`   | where to read from: `stdin`                                               |
| **return** | characters read (including `\n`), or `-1` on EOF or error                 |

Two details that confuse many beginners:

- You pass **addresses** (`&line`, `&cap`) because `getline` needs to *modify* your variables. A function cannot change a pointer you gave it by value. (Same idea as `t_node **` in [[linked_lists|linked lists]] §5.)
- Start with `line = NULL` and `cap = 0`. `getline` then allocates the first buffer itself.

```
 before the first call     after the first call
 line ──► NULL             line ──► [ 'l' 's' ' ' '-' 'l' ... '\n' '\0' ...]
 cap  = 0                  cap  = size of that buffer (not the line length!)
```

⚠️ `cap` is the **capacity** of the buffer, not the length of the line. The line length is the return value.

💡 On the next call, `getline` **reuses** the same buffer if it is big enough. That is why you do not free it inside the loop.

---

## 4. The shape of the loop

In words, without code:

```
 repeat forever:
     print the prompt
     read a line
     if reading failed (EOF) → leave the loop
     use the line
 after the loop:
     release the memory
```

### What is EOF?

Pressing **Ctrl-D** on an empty line tells the terminal "no more input". `getline` then returns `-1`. It is how a user leaves a shell without typing `exit`.

⚠️ **Ctrl-C is different.** It sends a *signal* that kills your program. Handling it nicely is lesson 08.

---

## 5. Two classic pitfalls

### ⚠️ The prompt does not appear

`stdout` is **buffered**: text is kept in memory until a newline or until the buffer fills. Your prompt `"> "` has no newline, so it can stay invisible while the program waits for input.

💡 Look up `fflush(stdout)` and decide where it belongs.

### ⚠️ Memory leaks

`getline` allocates with `malloc` (on the heap, see [[memory_&_c|memory and C]]). If you never `free` the buffer, a tool like Valgrind reports a leak. Decide **where** the single `free` goes: once per line, or once at the very end? Think about what `getline` does with a buffer that is already allocated.

---

## 6. Exercise

Write a program `mysh.c` that:

1. Prints a prompt `"> "`.
2. Reads a line with `getline`.
3. Prints the line back exactly as received.
4. Repeats until Ctrl-D, then exits with status `0`.
5. Has **no memory leak**.

Compile with:

```
gcc -Wall -Wextra -Werror mysh.c -o mysh
```

### Tests to run

| Input                                 | Expected behaviour                                      |
| ------------------------------------- | ------------------------------------------------------- |
| `hello⏎`                              | prints `hello` and a new prompt                         |
| `⏎` (empty line)                      | prints an empty line and a new prompt                   |
| `ls -l /tmp⏎`                         | prints `ls -l /tmp`                                     |
| a line of 5 000 characters (paste it) | printed in full, no crash                               |
| Ctrl-D                                | the program exits, the prompt does not loop forever     |
| `echo "hi" \| ./mysh`                 | works with piped input and exits at the end of the data |

### Check for leaks

```
sudo apt install valgrind
valgrind --leak-check=full ./mysh
```

Type a few lines, press Ctrl-D, and read the summary. You want `All heap blocks were freed -- no leaks are possible`.

---

## Check your understanding

1. Why do we pass `&line` and `&cap` to `getline` instead of `line` and `cap`?
2. After reading `"ls -l /tmp⏎"`, what is the last character before the terminating `'\0'`? Why does this matter for the next lesson?
3. What is the difference between pressing Ctrl-D and Ctrl-C at the prompt?
4. If you call `getline` 1 000 times and free only once at the end, is it a leak? Why or why not?

---

## What's next

In **[[02-splitting-a-line|Lesson 02]]** you turn the string `"ls -l /tmp\n"` into an array of words `["ls", "-l", "/tmp", NULL]`, the format that the system call used to launch programs expects.
