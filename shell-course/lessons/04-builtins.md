# Lesson 04 — Builtins: `cd` and `exit`

> ← [[03-running-a-program|Lesson 03]] · [[README|Course index]] · [[05-exit-status-and-errors|Lesson 05]] →

## Direct answer

Some commands **cannot** be run by `fork` + `execvp`, because they must change **the shell itself**:

- **`cd`** changes the *current directory of a process*. If a child did it, only the child's directory would change, and the child ends right after.
- **`exit`** ends the shell. A child cannot end its parent.

So the shell recognises these words **before** it forks, and runs them **inside its own process**. Such a command is a **builtin**.

By the end of this lesson, `cd /tmp` followed by `pwd` prints `/tmp`, and `exit` closes your shell.

---

## Running example

You type, one line after the other:

```
cd /tmp⏎
pwd⏎
```

With only lesson 03, the result is:

```
> cd /tmp
> pwd
/home/you/shell        ← not /tmp!
```

The goal of this lesson: understand **why**, and fix it.

---

## 1. Why `cd` fails as an external program

Try your shell with `cd /tmp`. It works without error (a `cd` program may exist on your system), yet nothing changes. Follow what happens:

```
 shell (PID 100, cwd = /home/you/shell)
    │ fork()
    ├────────────────────────────┐
    ▼                            ▼
 shell (PID 100)             child (PID 101, cwd = /home/you/shell)
    │                            │ execvp("cd", ...)
    │                            ▼
    │                         cd (PID 101)
    │                            │ chdir("/tmp")  → cwd of PID 101 = /tmp
    │                            │ ends
    │ waitpid(101)               ✗  the process, and its directory, are gone
    ◄────────────────────────────┘
    │
 shell (PID 100, cwd = /home/you/shell)    ← unchanged
```

The reason: **the current directory belongs to a process**, like its memory or its open files. A child gets a **copy** of it (you saw in [[03-running-a-program|lesson 03]] that `fork` copies). Changing the copy does not touch the parent's.

> A process can only change **its own** state. It cannot change its parent's.

Any command that must change the shell's own state is therefore built **into** the shell.

---

## 2. What is a builtin?

A builtin is a command that the shell executes **by itself**, with a normal C function call, and **without** `fork`.

| | External command (`ls`) | Builtin (`cd`) |
|---|---|---|
| Where is the code? | a separate program on disk, found in `$PATH` | a function **in your shell** |
| New process? | yes (`fork` + `exec`) | **no** |
| Can change the shell's state? | no | **yes** |
| Speed | slower (creates a process) | instant |

The decision is made for **each line**, right after splitting:

```
 words = ["cd", "/tmp", NULL]
            │
            ▼
   is words[0] one of my builtins?
        │                    │
       yes                   no
        │                    │
   call my function     fork + execvp   (lesson 03)
   in the shell itself
```

💡 Check how your own system sees them: in a real shell, `type cd` says *"cd is a shell builtin"*, and `type ls` says *"ls is /usr/bin/ls"*.

---

## 3. `chdir()`: the system call behind `cd`

```c
int chdir(const char *path);
```

| Returns | Meaning |
|---|---|
| `0` | success: the **calling process** is now in `path` |
| `-1` | failure: the directory is unchanged and `errno` says why |

Typical reasons for failure: the directory does not exist (`ENOENT`), is not a directory (`ENOTDIR`), or you may not enter it (`EACCES`). `perror` turns `errno` into text.

### What `cd` must handle

| Input | Expected behaviour |
|---|---|
| `cd /tmp` | go to `/tmp` |
| `cd` (no argument) | go to the **home directory**: read it with `getenv("HOME")` |
| `cd nosuchdir` | print an error with `perror`, **stay** where you are, keep running |
| `cd a b` (too many arguments) | print an error such as `cd: too many arguments`, do nothing |

`words[1]` is the argument. Because `words` ends with `NULL` (lesson 02), `words[1] == NULL` means "no argument". **Test `words[1]` before reading `words[2]`**: reading past the `NULL` is undefined behaviour.

⚠️ `getenv("HOME")` can return `NULL` (variable not set). Check it before passing it to `chdir`.

💡 `getenv` returns a pointer into the environment. You do **not** `free` it, and you should not modify it.

---

## 4. `exit`: ending the loop cleanly

`exit` has a different difficulty. You *could* call `exit(0)` right inside the builtin, but look at your loop:

```
 loop:
   print prompt
   line  = getline(...)      ← allocated by getline, freed after the loop
   words = split_line(line)  ← allocated every line, freed at the end of each turn
   run the command
   free(words)
```

If the builtin calls `exit(0)` in the middle, then at that moment `line` and `words` are still allocated. The OS reclaims everything when the process ends, but **`valgrind` reports it as a leak**, and in a bigger shell (open files, saved history) skipping cleanup causes real bugs.

The clean approach: the builtin **does not exit by itself**. It **tells the loop to stop**, and the loop exits normally through the cleanup code at the bottom.

```
 run the command ──► builtin "exit" ──► returns a signal "please stop"
                                              │
 loop sees the signal ─► break ─► free(words), free(line) ─► return
```

You have two common ways to send that signal:

| Option | How | Good | Bad |
|---|---|---|---|
| **Return value** | the builtin handler returns a number meaning "stop" | simple, no global state | every caller must pass the value up |
| **Global flag** | a global `int` the builtin sets and the loop checks | easy to wire | global state; harder to test; breaks if you later run builtins in children (lesson 07) |

**Why the return value here:** it keeps all state visible in function signatures, and the loop remains the only place that decides to stop.

**When to change:** if many functions deep in the call chain need to request a stop (for instance a parser that detects fatal errors), a flag, or `longjmp`, can be simpler than threading a return value through every level.

---

## 5. Where the check goes

You have two natural places for the builtin test:

```
 Option A: in main                     Option B: inside run_command
 ─────────────────────────────         ─────────────────────────────
 words = split_line(line)              words = split_line(line)
 if builtin → handle, continue         run_command(words)
 else       → run_command(words)         └─ if builtin → handle, return
                                          └─ else fork + exec
```

| | A: in `main` | B: in `run_command` |
|---|---|---|
| `main` stays readable | no, it grows | **yes** |
| One function = "run one command" | no | **yes** |
| How to tell `main` about `exit` | directly | return value |

**Why B here:** `run_command` becomes the single entry for "do what this line says", and `main` keeps its three-line loop. In lesson 07, pipelines will need exactly this shape.

**When to change:** a very small shell with only `exit` could keep the test in `main`.

Whichever you pick, the builtin test must happen **before** `fork`.

---

## 6. Complexity and structure

For two builtins, a pair of `strcmp` calls is perfectly fine. With `n` builtins, finding the right one costs **O(n)** string comparisons per line, which is nothing next to the cost of reading a line.

If you add many builtins later (`echo`, `pwd`, `export`, `unset`, `env`), a **table** keeps the code tidy:

```
 name      handler
 ──────    ─────────────
 "cd"      builtin_cd
 "exit"    builtin_exit
 "pwd"     builtin_pwd     ← later
```

Loop over the table, compare `words[0]` with `name`, call `handler`. You don't need this yet. Two `if` tests are simpler. Move to a table when adding a builtin means copying a block of code.

---

## 7. Pitfalls

- ⚠️ **Never `fork` for a builtin.** If you do, `cd` is back to lesson 03's bug.
- ⚠️ **Compare the whole word.** Use `strcmp(words[0], "cd") == 0`. `strncmp(words[0], "cd", 2)` would also match `cdrecord`.
- ⚠️ **Do not forget `<string.h>`** for `strcmp`.
- ⚠️ **Do not read `words[2]` before checking `words[1]`.** The array ends at the first `NULL`.
- ⚠️ **A failed `cd` must not stop the shell.** Print the error and carry on.
- ⚠️ **Do not call `exit()` from inside the builtin** unless you have freed `words` and `line` first. The cleaner option is the signal back to the loop (section 4).
- ⚠️ **Do not free `words` twice.** If the builtin returns "stop", the loop still has to free `words` once, not zero times and not twice. Trace it on paper.
- 💡 After `chdir`, `pwd` (an external program) prints the new directory. It reads the directory of **its own process**, which it inherited from your shell. That is the proof that your `cd` worked.
- 💡 Without a prompt showing the directory, you can't see where you are. A nice bonus: print `getcwd()` in the prompt.

---

## 8. Exercise

Extend your shell with two builtins, `cd` and `exit`.

1. In `run_command` (or `main`, your choice, see section 5), **before** `fork`, test whether `words[0]` is `cd` or `exit`.
2. `cd`:
   - with one argument: `chdir` to it;
   - with no argument: `chdir` to `$HOME` (handle an unset `HOME`);
   - with two or more arguments: print an error, do nothing;
   - if `chdir` fails: `perror`, and keep running.
3. `exit`: the shell stops **through the normal loop exit**, so that `words` and `line` are freed. `exit` takes no argument for now (lesson 05 adds the exit status).
4. Every other command still goes through `fork` + `execvp`.
5. Update `shell.h` if you add or change prototypes.
6. No memory leak, including when the shell ends with `exit`.

💡 Decide first, on paper: what does `run_command` return, and what does `main` do with it? Then write the code.

Compile with:

```
gcc -Wall -Wextra -Werror mysh.c split.c run.c -o mysh
```

### Tests to run

| Input | Expected behaviour |
|---|---|
| `cd /tmp⏎` then `pwd⏎` | prints `/tmp` |
| `cd⏎` then `pwd⏎` | prints your home directory |
| `cd ..⏎` then `pwd⏎` | prints the parent of the previous directory |
| `cd nosuchdir⏎` | error message, **shell still running** |
| `cd /tmp /usr⏎` | error about arguments, directory unchanged |
| `cd /etc/passwd⏎` | error (not a directory), shell still running |
| `exit⏎` | shell ends, back in your usual terminal |
| `exit⏎` then `echo $?` | prints `0` |
| Ctrl-D | still ends the shell, as before |
| `ls⏎` after a `cd` | lists the **new** directory |
| `cdrecord⏎` | treated as an external command (not as `cd`) |

### Leak check

```
valgrind --leak-check=full ./mysh
```

Type a few commands, including a failing `cd`, then `exit`. Then do the same run ending with Ctrl-D. Both runs must report `All heap blocks were freed -- no leaks are possible`.

### Prove the theory

Temporarily comment out your `cd` builtin and try `cd /tmp` then `pwd`. You should see the bug from section 1 (with a shell on Linux there may be no `cd` program at all; the error message is also instructive). Restore the builtin afterwards.

---

## Check your understanding

1. Why can't `cd` work as an external program? Which property of a process explains it?
2. After `fork`, the child calls `chdir("/tmp")`. What is the current directory of the parent?
3. Why does `pwd` print the right directory after your builtin `cd`, even though `pwd` is an external program?
4. What does `exit` risk leaving behind if the builtin calls `exit(0)` immediately? Which tool shows it?
5. What is the difference between `strcmp(words[0], "cd")` and `strncmp(words[0], "cd", 2)`, and which input shows it?
6. `cd` with no argument needs `getenv("HOME")`. What can go wrong with the returned pointer?
7. For `n` builtins and a line, what is the cost of finding the right builtin with simple comparisons? Does it matter?

---

## Related notes

- [[03-running-a-program]]: `fork` copies the child's state, which is why a child's `chdir` is lost.
- [[02-splitting-a-line]]: why `words[1]` is `NULL` when there is no argument.
- [[00-setup-and-mental-model]]: the shell as a process that manages other processes.
- [[memory_&_c]]: freeing `words` and `line` on every exit path.

---

## What's next

In **[[05-exit-status-and-errors|Lesson 05]]** you learn how a program tells the shell whether it succeeded: the **exit status**. You read it with `WEXITSTATUS`, store it for `$?`, make `exit 3` return 3, and report errors in a consistent way with `perror`.
