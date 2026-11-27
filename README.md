# Build a Shell in C

> A hands-on course for beginners. You write the shell yourself, one small step at a time. The lessons give you the ideas, the diagrams and the pitfalls, but **never the full solution**.

---

## Who is this for?

Anyone who knows the basics of C (variables, functions, pointers, `malloc`/`free`) and wants to understand what really happens when you type `ls -l /tmp` in a terminal.

You do **not** need to know anything about processes, system calls or file descriptors. The course introduces each concept before using it.

## How the course works

- Every lesson starts with a **direct answer** to one question.
- Every lesson uses **one running example**: the command `ls -l /tmp`.
- Every lesson ends with **an exercise where you write the code**, plus questions to check your understanding.
- Code snippets in the lessons are short and only illustrate an idea. Writing the actual shell is your job.

## Requirements

| You need | Why |
|---|---|
| Linux, macOS, or **WSL** on Windows | The shell relies on `fork()`, which does not exist on native Windows |
| `gcc` or `cc` | To compile your code |
| A text editor | Any will do |

⚠️ If you are on Windows, install WSL first (`wsl --install` in an administrator terminal) and do all the exercises inside it.

## Lessons

| # | Lesson | You build | Key concept |
|---|---|---|---|
| 00 | [[00-setup-and-mental-model\|Setup & mental model]] | a compiled `hello.c` | what a shell, a process and a system call are |
| 01 | [[01-the-read-loop\|The read loop]] | a prompt that echoes your input | `stdin`, `getline`, dynamic memory |
| 02 | [[02-splitting-a-line\|Splitting a line]] | `"ls -l /tmp"` → `["ls","-l","/tmp",NULL]` | arrays of pointers, `argv` |
| 03 | [[03-running-a-program\|Running a program]] | your shell launches `ls` | `fork`, `execvp`, `waitpid` |
| 04 | [[04-builtins\|Builtins]] | `cd` and `exit` | why `cd` cannot be an external program |
| 05 | Exit status & errors | `$?` and error messages | `status`, `perror`, `WEXITSTATUS` |
| 06 | Redirections | `>`, `>>`, `<` | file descriptors, `open`, `dup2` |
| 07 | Pipes | `ls \| wc -l` | `pipe`, closing fds, two children |
| 08 | Signals *(optional)* | Ctrl-C that does not kill your shell | `sigaction` |
| 09 | Quotes & variables *(optional)* | `"a b"`, `$HOME` | a real parser |

*Lessons without a link are not written yet. Links are Obsidian wikilinks (`\|` is the alias separator inside tables).*

## Conventions

- ⚠️ marks a **pitfall**: something that commonly breaks.
- 💡 marks a **tip**.
- Diagrams are plain ASCII so they render everywhere.
- Complexity is given in Big O when it matters.

## Project layout

```
shell-course/
├── README.md
└── lessons/
    ├── 00-setup-and-mental-model.md
    └── ...
```

Your own shell code lives **outside** this repository (or in your own fork), so the course stays reusable by everyone.

## License

Choose a license before publishing (for example MIT for the text and code snippets, or CC BY 4.0 for the lessons).
