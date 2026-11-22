# Lesson 02 — Splitting a line

> ← [[01-the-read-loop|Lesson 01]] · [[README|Course index]] · [[03-running-a-program|Lesson 03]] →

## Direct answer

To run `ls -l /tmp`, the system needs **three separate words**, not one string. So your shell must turn `"ls -l /tmp\n"` into an **array of pointers to words, ending with `NULL`**: the exact shape of `argv` in `main`.

By the end of this lesson your shell will print each word of every line you type on its own line.

---

## Running example

You type:

```
ls -l /tmp⏎
```

[[01-the-read-loop|Lesson 01]] gave you the string `"ls -l /tmp\n"` in `line`. The goal of this lesson:

```
 "ls -l /tmp\n"     ──►     ["ls", "-l", "/tmp", NULL]
  one string                 four pointers (the last one is NULL)
```

---

## 1. The target shape: `char **`

An array of words is an array of `char *`, so its type is `char **`. You already know this shape: it is `argv`.

```
 words ──► ┌───────┐
           │   ●───┼──► "ls"
           ├───────┤
           │   ●───┼──► "-l"
           ├───────┤
           │   ●───┼──► "/tmp"
           ├───────┤
           │   ╳   │      ← NULL: "no more words"
           └───────┘
```

Three rules to remember:

- **The last pointer is `NULL`.** The array has no stored length: whoever reads it stops at `NULL`. That is exactly what `execvp` will do in lesson 03.
- For 3 words you need **4 slots** (`count + 1`).
- An **empty line** gives an array containing only `NULL`. Your shell must then do nothing and show the prompt again.

💡 If `char **` still feels foggy, the pointer-to-pointer idea is explained with boxes in [[linked_lists|linked lists]] (§5, "Le pointeur de pointeur") and ownership rules ("who frees what?") in [[memory_&_c#8. Les pièges (check-list avant de rendre)|memory and C §8]].

---

## 2. What is a word?

Define it once, precisely:

> A **word** is a maximal run of characters that are **not** separators.

Separators for now: **space, tab and newline** (`' '`, `'\t'`, `'\n'`).

| Input | Words |
|---|---|
| `ls -l /tmp\n` | `ls`, `-l`, `/tmp` |
| `␠␠ls␠␠␠-l␠\n` (extra spaces) | `ls`, `-l` |
| `ls\t-l\n` (a tab) | `ls`, `-l` |
| `\n` (empty line) | *(none)* |
| `␠␠␠\n` (only spaces) | *(none)* |

⚠️ Notice the `\n`. It is the last character `getline` gave you (question 2 of [[01-the-read-loop|lesson 01]]). If it is not a separator, your last word becomes `"/tmp\n"` and `ls` looks for a directory with a newline in its name.

---

## 3. Three ways to split

### ① `strtok`

The library function that cuts a string at every separator.

```
 before:  l s ␠ - l ␠ / t m p \n \0
 after:   l s \0 - l \0 / t m p \0 \0     ← strtok wrote '\0' over the separators
           ▲        ▲       ▲
           └ words point INTO the original string
```

- ✅ Short to write.
- ✅ The words are not copied: only the array of pointers needs `malloc`.
- ❌ It **keeps hidden state** between calls (a static pointer): not reentrant, and one bug can corrupt the next call.
- ❌ It **destroys the original line**.
- ❌ Some schools forbid it.

### ② Your own scan, in place (what `strtok` does, but under your control)

Walk through the string once with an index. Two states: *outside a word* and *inside a word*.

```
 index : 0 1 2 3 4 5 6 7 8 9 10 11
 char  : l s ␠ - l ␠ / t m p \n \0
         │     │     │
         └─ word starts here when we go from "outside" to "inside"
```

When a word ends, write `'\0'` over the separator and store the pointer to the word's first character. Do **two passes**: the first counts the words (so you know how big the array must be), the second fills it.

```
 words[0] ──► index 0  → "ls"
 words[1] ──► index 3  → "-l"
 words[2] ──► index 6  → "/tmp"
 words[3] =   NULL
```

- ✅ No hidden state, you control every step.
- ✅ **One** `malloc` (the array) and **one** `free`.
- ❌ The words live **inside `line`**: they become invalid as soon as `line` is reused (next `getline`) or freed.
- ❌ You write the scan yourself, so off-by-one bugs are possible.

### ③ Your own scan, copying each word

Same scan, but each word is copied into its own `malloc`'d string (think `strndup`, or `ft_split` if you wrote one).

```
 line  ──► [ l s ␠ - l ␠ / t m p \n \0 ]      (untouched)

 words ──► ┌───┐   ┌────────┐
           │ ● ├──►│ l s \0 │
           │ ● ├─┐ └────────┘   ┌────────┐
           │ ● ├─┼─────────────►│ - l \0 │
           │ ╳ │ │              └────────┘
           └───┘ └──► ... one malloc per word
```

- ✅ The words are **independent** of `line`: they survive the next `getline`.
- ❌ `count + 1` allocations, so `count + 1` frees, and a failure halfway through needs a **clean-up of what was already allocated**.
- ❌ More code, more leak risk.

### Comparison

| | ① `strtok` | ② scan in place | ③ scan + copy |
|---|---|---|---|
| Time | O(n) | O(n) | O(n) |
| Extra memory | array only | array only | array + all words |
| `malloc` calls | 1 | 1 | count + 1 |
| `free` calls | 1 | 1 | count + 1 |
| Destroys `line` | yes | yes | no |
| Words survive next `getline` | no | no | **yes** |
| Difficulty | ⭐ | ⭐⭐ | ⭐⭐⭐ |
| Risk of bug | hidden state | off-by-one | leaks on failure |

---

## 4. Why ② for this course

1. **One owner of the text.** `line` owns the characters, and `words` only points into it. Rule: `free(words)` frees the array, never the words.
2. **It matches the shell's life cycle.** Read a line, split it, run the command, then read the next line. The words are no longer needed when `getline` overwrites the buffer, so their short life is not a problem.
3. **Cheap and predictable.** Two passes over a line are 2 × n steps. A line of 5 000 characters costs about 10 000 steps, which is nothing.
4. **You see how `strtok` works**, and you can explain it without using it.

## 5. When to change

| Situation | Better option |
|---|---|
| You keep words after the next `getline` (command history, a list of jobs) | ③ copy each word |
| You need to build words that are not in the original text (`$HOME` replaced by `/home/you`) | ③ the new word is a new string |
| Quotes: `echo "a b"` must stay **one** word | a real parser, see lesson 09 |

---

## 6. Pitfalls

- ⚠️ **Array size.** `count` words need `count + 1` slots. Forgetting the `NULL` slot is the classic bug.
- ⚠️ **Leading, trailing and repeated spaces.** Splitting at every single space gives empty words.
- ⚠️ **Empty line.** `count == 0` must give `[NULL]`, not a crash, and the shell must not try to run anything.
- ⚠️ **`printf("%s", line)` after splitting in place** prints only `ls`: the first separator is now `'\0'`.
- ⚠️ **Never `free` a word** in option ②: it points into the middle of `line`.
- ⚠️ **`malloc` can fail.** Check its result.
- 💡 To debug, write a tiny loop that prints `words[i]` until `NULL`. If it never stops, you forgot the `NULL`.

---

## 7. Exercise

Extend `mysh.c` (or put the new code in `split.c`) so that, after `getline`:

1. A function `char **split_line(char *line)` returns the array of words described above, or `NULL` if `malloc` fails.
2. The shell prints every word as `[word]` on its own line.
3. The array is freed before the next prompt.
4. Empty and blank lines print nothing and show a new prompt.
5. No memory leak.

💡 Write a helper that **counts the words** first and test it alone. Then write the filling pass.

Compile with:

```
gcc -Wall -Wextra -Werror mysh.c -o mysh
```

### Tests to run

| Input | Expected output |
|---|---|
| `ls -l /tmp⏎` | `[ls]` `[-l]` `[/tmp]` |
| `␠␠ls␠␠␠-l␠␠⏎` | `[ls]` `[-l]` |
| `ls⇥-l⏎` (a tab) | `[ls]` `[-l]` |
| `hello⏎` | `[hello]` |
| `⏎` | nothing, new prompt |
| `␠␠␠⏎` | nothing, new prompt |
| 2 500 words separated by spaces (paste) | all printed, no crash |
| Ctrl-D | exits with status `0` |

### Check for leaks

```
valgrind --leak-check=full ./mysh
```

Type several lines (including an empty one), then Ctrl-D. You want `All heap blocks were freed -- no leaks are possible`.

---

## Check your understanding

1. Why must the array end with `NULL`, and how does the code that reads it know how many words there are?
2. After an in-place split, what does `printf("%s", line)` print, and why?
3. Why do you call `free(words)` but never `free(words[0])` in option ②?
4. After the next `getline`, what has happened to the pointers in `words`? Why is that not a problem in this shell?
5. What would a naive split on every single `' '` give for `"ls  -l"` (two spaces)?

---

## Related notes

- [[01-the-read-loop]]: where `line` comes from and why `getline` reuses its buffer.
- [[linked_lists]]: pointer-to-pointer, the same idea as `char **`.
- [[memory_&_c]]: stack, heap and "who frees what?".
- [[push_swap#2. Le parsing|push_swap parsing]]: the same problem, splitting `"3 2 1"` with `ft_split`.

---

## What's next

In **[[03-running-a-program|Lesson 03]]** you give this array to `execvp` and your shell finally launches `ls`. You will meet `fork`, `execvp` and `waitpid`.
