# Lesson 03 — Running a program

> ← [[02-splitting-a-line|Lesson 02]] · [[README|Course index]] · [[04-builtins|Lesson 04]] →

## Direct answer

To run `ls -l /tmp`, your shell does three things with three system calls:

1. **`fork()`**: create a copy of the shell (a child process).
2. **`execvp()`**: in the child only, replace the copy with the program `ls`.
3. **`waitpid()`**: in the parent (the shell), wait until the child is finished.

By the end of this lesson, typing `ls -l /tmp` in your shell prints the content of `/tmp`, then shows the prompt again.

---

## Running example

You type:

```
ls -l /tmp⏎
```

[[02-splitting-a-line|Lesson 02]] gave you:

```
 words ──► ["ls", "-l", "/tmp", NULL]
```

The goal of this lesson: turn this array into a **running process**.

---

## 1. Why two steps (`fork` then `exec`)?

You saw the reason in [[00-setup-and-mental-model|lesson 00]]: if the shell turned itself into `ls`, the shell would **disappear** when `ls` ends. So Unix separates two ideas:

| Call | What it does | What it does **not** do |
|---|---|---|
| `fork` | duplicates the current process | does not start any new program |
| `execvp` | replaces the current process with a program | does not create any process |

```
 shell (PID 100)
    │
    │ fork()
    ├──────────────────────┐
    ▼                      ▼
 shell (PID 100)        copy of the shell (PID 101)
 "I am the parent"      "I am the child"
    │                      │ execvp("ls", ...)
    │                      ▼
    │                   ls (still PID 101!)
    │ waitpid(101)         │
    │ ... waits ...        │ prints, then finishes
    ◄──────────────────────┘
    │
 prompt again
```

💡 `exec` does not change the PID. It is the same process, with a new program inside.

---

## 2. `fork()`: one call, two returns

```c
pid_t fork(void);
```

After a successful `fork`, **two processes run the same code, from the same line**. The only difference is the return value:

| Who you are | `fork()` returns |
|---|---|
| the **child** | `0` |
| the **parent** | the child's PID (a positive number) |
| nobody (failure, no child created) | `-1` |

This is how one piece of code can behave differently in the two processes. In words:

```
 pid = fork()
 if pid is -1  → error: report it, do not run anything
 if pid is 0   → I am the child: this is where I become `ls`
 otherwise     → I am the parent: this is where I wait
```

### What exactly is copied?

The child gets a copy of the parent's memory (your `line`, your `words`, your variables) and of its open file descriptors. Modern systems copy lazily (*copy-on-write*), so a `fork` is cheap.

```
 parent                      child
 line  ──► "ls\0-l\0/tmp\0"  line  ──► "ls\0-l\0/tmp\0"    (a separate copy)
 words ──► [●,●,●,╳]         words ──► [●,●,●,╳]
```

⚠️ **Copy, not shared.** If the child changes a variable, the parent does not see it.

---

## 3. `execvp()`: become another program

```c
int execvp(const char *file, char *const argv[]);
```

| Parameter | Meaning |
|---|---|
| `file` | the program name. `execvp` searches for it in every directory of `$PATH` |
| `argv` | **your array from lesson 02**: words, ending with `NULL` |

Now you see why the array needs a `NULL` and why `words[0]` is repeated: you give `file = words[0]` **and** `argv = words`. The program receives `words` as its own `argv`.

```
 execvp(words[0], words)
        │          │
        │          └─► becomes `argv` inside ls's main()
        └─ "ls": searched in /usr/local/bin, /usr/bin, /bin, ...
```

### What the `v` and `p` mean

| Letter | Meaning |
|---|---|
| `v` | arguments given as a **v**ector (an array), which is what you have |
| `p` | the program is searched in **$P**ATH |

(`execl` takes a fixed list instead of an array. `execv` does not search `$PATH`, so you would have to type `/bin/ls`.)

### The most important property

> **If `execvp` works, it never returns.**

The old program (your shell's copy) no longer exists after the call: the code that follows is gone. So **the only way to reach the line after `execvp` is that it failed**, for example because the command does not exist.

⚠️ In the child, the line after a failed `execvp` must **end the child** (`exit` or `_exit`). If you forget, the child continues the shell loop, and you get **two shells reading the same keyboard**. A classic bug.

---

## 4. `waitpid()`: wait for the child

```c
pid_t waitpid(pid_t pid, int *status, int options);
```

| Parameter | Meaning |
|---|---|
| `pid` | which child to wait for (the value `fork` gave the parent) |
| `status` | address of an `int` where the system writes **how the child ended** |
| `options` | `0` for now: block until the child ends |

Without `waitpid`, two things go wrong:

1. The prompt appears **before** `ls` has finished printing.
2. The finished child stays in the process table as a **zombie** until the parent collects it.

```
 without waitpid                       with waitpid
 > ls -l /tmp                          > ls -l /tmp
 > total 12                            total 12
 drwx------ ...                        drwx------ ...
                                       >
 (prompt in the middle of the output)  (prompt after the output)
```

For now, **ignore** the value written in `status`. Lesson 05 explains how to read it (`WEXITSTATUS`).

---

## 5. Putting the three together

In words, for **one** line:

```
 split the line into words
 if there are no words           → do nothing, new prompt
 pid = fork()
   child:  execvp(words[0], words)
           if we are still here → execvp failed: say so, end the child
   parent: waitpid(pid, ...)
 free(words)
```

And the memory rules do not change:

| What | Who frees it | When |
|---|---|---|
| `words` (the array) | the parent shell | after `waitpid`, once per line |
| `line` | the parent shell | once, after the loop |
| the child's copies | nobody: they vanish when the child ends or is replaced by `exec` | |

💡 You do not need to free anything in the child before `execvp`. The system throws the whole old memory away.

---

## 6. Three ways to run a program

| | `system("ls -l /tmp")` | `fork` + `execvp` | `posix_spawn` |
|---|---|---|---|
| Lines of code | 1 | ~10 | ~10 |
| Who parses the line | **another shell** (`/bin/sh`) | you (lesson 02) | you |
| Control over redirections / pipes | none | **full** | partial |
| Teaches how a shell works | no, you would write a shell with a shell | **yes** | a bit |
| Risk | command injection | bugs in your code | rare |

**Why `fork` + `execvp` here:** between `fork` and `execvp` you can modify the child (change its file descriptors). That gap is exactly what makes `>` and `|` possible in lessons 06 and 07. `system` and `posix_spawn` hide it.

**When to change:** a program that only launches one command and has no shell to write (a build tool, a daemon manager) can use `posix_spawn`. It is faster on huge processes and less error-prone.

---

## 7. Pitfalls

- ⚠️ **The child must not return into your loop.** After a failed `execvp`, end the child.
- ⚠️ **Do not forget to wait**, or you have zombies and a prompt in the wrong place.
- ⚠️ **Do not wait if `fork` returned `-1`**: there is no child.
- ⚠️ **Buffered output is duplicated by `fork`.** If your prompt text is still in the `stdout` buffer at the moment of `fork`, both processes may print it. Keep your `fflush(stdout)` after the prompt, and flush before `fork` if you print anything else.
- ⚠️ **Never run `execvp` in the parent.** Your shell would be replaced and the terminal would close (try it once on purpose to see).
- ⚠️ **`words[0]` must exist.** An empty line has `words[0] == NULL`: skip `fork` entirely.
- ⚠️ **Do not call `fork` in an infinite loop by mistake.** A `fork` whose children also `fork` is a **fork bomb**. Make sure each child either becomes a program or ends.
- 💡 Use `perror("mysh")` to report a failed `execvp`: it prints your text and the reason (`mysh: No such file or directory`). More in lesson 05.

---

## 8. Exercise

Extend `mysh.c` (put the new code in a function `run_command(char **words)` in a new file `run.c` if you like) so that:

1. After splitting, an **empty** line does nothing.
2. Otherwise the shell **forks**; the child runs `execvp(words[0], words)`.
3. If `execvp` fails, the child prints an error message on `stderr` and ends.
4. The parent **waits** for the child before showing the next prompt.
5. If `fork` fails, the shell prints an error and goes on with the next prompt.
6. No memory leak. The `[word]` printing of lesson 02 can be removed.

💡 Remember to add the new headers: `<unistd.h>` (`fork`, `execvp`) and `<sys/wait.h>` (`waitpid`). Add the new prototype to `shell.h`.

Compile with:

```
gcc -Wall -Wextra -Werror mysh.c split.c -o mysh
```

(Add `run.c` if you created it.)

### Tests to run

| Input | Expected behaviour |
|---|---|
| `ls⏎` | lists the current directory, then the prompt |
| `ls -l /tmp⏎` | long listing of `/tmp`, **then** the prompt |
| `␠␠echo␠␠hello␠␠world⏎` | `hello world` (spaces cleaned by your split) |
| `pwd⏎` | prints the current directory |
| `⏎` | new prompt, no new process |
| `nosuchcommand⏎` | an error message, then a new prompt (**the shell is still alive**) |
| `sleep 3⏎` | the prompt comes back only after 3 seconds |
| `ls /nonexistent⏎` | the error comes from `ls`; your shell goes on |
| Ctrl-D | exits with status `0` |

### Observe the process tree

While `sleep 30` runs in your shell, open a second terminal and run:

```
ps -o pid,ppid,cmd --forest
```

You should see `mysh` with a child `sleep 30`, and the child's **PPID** (parent PID) equals the PID of `mysh`. This is the diagram of section 1, for real.

### Check for leaks and zombies

```
valgrind --leak-check=full --trace-children=yes ./mysh
```

Run several commands, then Ctrl-D. You want no leak in the shell. While your shell waits at the prompt, `ps` must not show `<defunct>` processes.

---

## Check your understanding

1. `fork` is called once but returns twice. In which processes, and with which values?
2. What does the child have in memory right after `fork`? If it changes `words[0]`, does the parent see it?
3. Why can the line after `execvp` only be reached when something went wrong?
4. What would happen if your shell called `execvp` directly, without `fork`?
5. What is a zombie process, and which call prevents it?
6. If you forget to end the child after a failed `execvp`, what do you see on the screen when you type `nosuchcommand`?

---

## Related notes

- [[02-splitting-a-line]]: where the `words` array and its final `NULL` come from.
- [[00-setup-and-mental-model]]: why the shell copies itself before running a program.
- [[memory_&_c]]: why the child's memory is a copy and not shared.

---

## What's next

In **[[04-builtins|Lesson 04]]** you meet commands that **cannot** work with `fork` + `execvp`: `cd` and `exit`. You will see why `cd` must run **inside** the shell process, and how your shell recognises such commands before launching anything.
