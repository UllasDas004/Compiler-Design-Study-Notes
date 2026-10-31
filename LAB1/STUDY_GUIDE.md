# LAB 1 — Complete Study Guide
## Two-Buffer System & Full Lexical Analyzer

---

## Files At a Glance

| File | Purpose | Focus |
|------|---------|-------|
| `que1.cpp` | Two-buffer demo lexer | Buffer switching mechanism, sentinel (`\0`) |
| `que2.cpp` | Full Lexical Analyzer | All token types, tabular output, sentinel (`$`) |

---

# QUESTION 1: Two-Buffer Input System (`que1.cpp`)

---

## The Core Problem: Why Not Just Read One Char at a Time?

When a compiler reads `fgetc()` for every character, each call crosses into the **OS kernel** (a system call). On a 100,000-line file, that's millions of slow round-trips. The solution: read a big **chunk** at once into a buffer (one OS call → thousands of characters). But chunking raises a new problem:

> **What happens if a token spans the boundary between two chunks?**

e.g., Buffer 1 ends with `...in` and Buffer 2 starts with `t...`. The token is `int` but it straddles the boundary.

The **two-buffer system** solves this by keeping both chunks in memory simultaneously, so the scanner can cross the boundary seamlessly.

---

## Memory Layout

```
BUFFER_SIZE = 8

buffer1:  [ i ][ n ][ t ][ ' '][ m ][ a ][ i ][ n ][ \0 ]
           0    1    2    3     4    5    6    7    8(sentinel)
           ^
           forwardPtr starts here

buffer2:  [ ( ][ ) ][ \n][ { ][ \n][ ' '][ i ][ n ][ \0 ]
           0    1    2    3    4    5     6    7    8(sentinel)
```

When `forwardPtr` reaches index 8 and reads `\0` (the sentinel), it loads new data into `buffer2` (or `buffer1`) and resets `forwardPtr` to the start of that buffer.

---

## Algorithm for `que1.cpp` — Step by Step

### PHASE 1: Setup

```
1. Open the file "input.txt"
2. loadBuffer(buffer1)  → reads first 8 chars from file into buffer1, puts '\0' at position [n]
3. loadBuffer(buffer2)  → reads next 8 chars from file into buffer2, puts '\0' at position [n]
   WHY load buffer2 now? So when forwardPtr hits the sentinel in buffer1,
   buffer2 is already populated and ready. No re-read needed at switching time.
4. forwardPtr  = buffer1  → scanner starts at the beginning of buffer1
5. lexemeBegin = buffer1  → marks where the current token started
6. lexeme = ""            → empty string to accumulate the current token
```

### PHASE 2: The Read Loop (line by line walk-through of main's while loop)

```
LOOP FOREVER:
    c = getChar()              [Call the character reader]

    IF c == '\0':
        BREAK                  [File exhausted — stop scanning]

    IF c is alphanumeric OR c is '_':
        lexeme += c            [This char belongs to an identifier/keyword]
                               [WHY _ ? C identifiers can contain underscores]
                               [WHY alphanumeric? identifiers can have digits AFTER first char]

    ELSE:
        [This is NOT an identifier character — it's a boundary (space, +, ;, \n, etc.)]
        IF lexeme is NOT empty:
            [We just completed a word token]
            IF isKeyword(lexeme):
                PRINT lexeme + "-> Keyword"
            ELSE:
                PRINT lexeme + "-> Identifier"
            lexeme.clear()     [Reset for the next token]
        [The non-alphanumeric char itself (operator, space) is IGNORED in this simple version]

AFTER LOOP:
    IF lexeme is NOT empty:    [Handle the very last token (no trailing whitespace case)]
        Classify and print it
```

### PHASE 3: getChar() — The Core Buffer-Switching Function (Ping-Pong Buffering)

```
getChar():
    ch = *forwardPtr           [Read the char at current position]

    IF ch == '\0':             [HIT THE SENTINEL — current buffer is completely read]
        IF usingFirst:
            forwardPtr = buffer2      [1. Switch immediately to buffer2 (already preloaded & waiting!)]
            usingFirst = false        [2. Flag that we're now reading buffer2]
            loadBuffer(buffer1)       [3. Refill the consumed buffer1 with next chunk from file]
        ELSE:
            forwardPtr = buffer1      [1. Switch immediately to buffer1 (already preloaded & waiting!)]
            usingFirst = true         [2. Flag that we're now reading buffer1]
            loadBuffer(buffer2)       [3. Refill the consumed buffer2 with next chunk from file]

        ch = *forwardPtr       [Read first char of the newly active buffer]

    forwardPtr++               [Advance for the NEXT call]
    RETURN ch
```

> **CRITICAL CONCEPT: Which buffer do we refill?**
> - Both `buffer1` and `buffer2` were loaded at the start in `main()`.
> - When `buffer1` hits sentinel, `buffer2` is **already waiting with the next chunk of text**.
> - Therefore, we switch to `buffer2` **instantly** (zero delay).
> - Then we reload `buffer1` (the one we just finished) so it will be ready when `buffer2` runs out!
> - In a real operating system / compiler, this refill of the inactive buffer happens **asynchronously in the background** via DMA / background I/O while the CPU scans the active buffer.
> - **Never call `loadBuffer(buffer2)` when switching to `buffer2`**, because that would overwrite the preloaded data before you even read it!

> **WHY `forwardPtr++` AFTER reading, not before?**
> Because we want to read the char AT the current position, then advance past it.
> If we did `++forwardPtr` first, we'd skip the first character of every buffer.

### PHASE 4: isKeyword() — Classify After Full Collection

```
isKeyword(string s):
    keywords = ["int","float","char","if","else","while","for","return","void","double"]
    FOR each keyword k:
        IF s == k: RETURN true
    RETURN false
```

> **WHY classify AFTER collecting the whole word (not during)?**
> Because `int` is a keyword but `integer` is an identifier.
> If you classified mid-stream (after seeing `i`, `n`, `t`), you'd wrongly
> match `int` inside `integer`. Maximal munch says: read the longest possible
> valid token FIRST, then classify.

---

## The `loadBuffer()` Function Explained

```cpp
void loadBuffer(char buffer[]) {
    fin.read(buffer, BUFFER_SIZE);   // ask OS for BUFFER_SIZE bytes
    int n = fin.gcount();            // how many bytes actually arrived
    buffer[n] = '\0';                // sentinel goes right after real data
}
```

**Three cases:**
| Scenario | `n` value | Sentinel position |
|----------|-----------|-------------------|
| Full read | 8 | `buffer[8]` |
| Partial (near EOF) | 3 (e.g.) | `buffer[3]` |
| EOF, nothing left | 0 | `buffer[0]` → immediate stop |

When `n = 0`: `buffer[0] = '\0'`. `getChar()` reads this sentinel immediately and returns `'\0'`. The main loop sees `'\0'` and breaks. **File processing is complete.**

---

## The "Last Token" Problem

Consider the file content: `int main` (no newline at the end).

```
Scan 'i': lexeme = "i"
Scan 'n': lexeme = "in"
Scan 't': lexeme = "int"
Scan ' ': SPACE → classify "int" → print "int-> Keyword", clear lexeme
Scan 'm': lexeme = "m"
Scan 'a': lexeme = "ma"
Scan 'i': lexeme = "mai"
Scan 'n': lexeme = "main"
Scan '\0': BREAK  ← we exit WITHOUT ever printing "main"!
```

That's why after the loop we have:
```cpp
if(!lexeme.empty()) {
    // classify and print the last accumulated token
}
```

---

## Sample Output

**input.txt contains:** `int x float`

```
int-> Keyword
x-> Identifier
float-> Keyword
```

*(Note: `que1.cpp` only classifies keywords and identifiers — numbers and operators are skipped)*

---
---

# QUESTION 2: Full Lexical Analyzer (`que2.cpp`)

---

## How This Extends que1

| Feature | que1 | que2 |
|---------|------|------|
| Sentinel character | `\0` | `$` |
| Buffer switching | Explicit via `getChar()` | Outer loop with flag `firstBuffer` |
| Token types | keyword, identifier | ALL: keyword, identifier, constant, literal, operator, delimiter |
| Maximal munch | Not implemented | YES for `==`, `!=`, `<=`, `>=` |
| Line tracking | No | Yes |
| Output format | Plain | Formatted table with `setw()` |
| Comments | Not handled | `//` comments are skipped |

---

## Why `$` Instead of `\0` as Sentinel?

Both `\0` and `$` work as sentinels if your source language doesn't use them.
- `\0` can't appear in normal text files, so it's very safe.
- `$` is more visible when debugging (you can print it), but technically valid in some languages.

The key rule: **the sentinel must be a character that CANNOT appear in valid source code.**

---

## Algorithm for `que2.cpp` — Complete, Decision by Decision

### OUTER LOOP: File Chunk Processing

```
INITIALIZE:
    firstBuffer = true
    line = 1
    Print table header

OUTER LOOP (while file has data):
    IF firstBuffer:
        file.read(buffer1, BUFFER_SIZE)
    ELSE:
        file.read(buffer2, BUFFER_SIZE)

    count = gcount()            [bytes actually read]
    IF count == 0: BREAK        [nothing left in file]

    buffer[count] = '$'         [place sentinel RIGHT AFTER real data]

    current = (firstBuffer) ? buffer1 : buffer2
    i = 0

    INNER LOOP (while current[i] != '$'):
        ... process characters ...

    firstBuffer = !firstBuffer  [toggle for next chunk]
```

### INNER LOOP: Character-by-Character Token Detection

The inner loop processes characters **sequentially in priority order**. The first matching `if` wins for each character.

---

#### Step A: Newline `\n`
```
IF ch == '\n':
    line++      ← WHY before isspace()? Because isspace('\n') = true,
                  and we'd lose the line count if we just "skip whitespace".
    i++
    continue
```

---

#### Step B: Whitespace (space, tab)
```
IF isspace(ch):
    i++         ← Not a token. Just skip.
    continue
```

---

#### Step C: Single-Line Comment `//`
```
IF ch == '/' AND current[i+1] == '/':
    WHILE current[i] != '\n' AND current[i] != '$':
        i++     ← Skip everything until end of line

    continue    ← DON'T do i++ here! Leave i pointing at '\n' so
                  the OUTER iteration picks it up and increments line counter.
```

> **Decision rationale:** Why check `current[i+1]`? Because `/` alone is a valid
> division operator. We only skip if BOTH characters are `/`.
> Checking `i+1` is safe because `$` is not `/`, so even at buffer end it fails correctly.

---

#### Step D: Identifier or Keyword
```
IF isalpha(ch) OR ch == '_':
    word = ""
    WHILE current[i] is alphanumeric OR '_':
        word += current[i]
        i++
        ← i advances INSIDE this loop, so after it, i points past the word

    IF isKeyword(word):
        print line, word, "Keyword"
    ELSE:
        print line, word, "Identifier"

    continue    ← i is already past this token
```

**Maximal munch here:** We greedily consume ALL alphanumeric chars.
- `int` → Keyword ✓
- `integer` → Identifier ✓ (not split into `int` + `eger`)
- `if1` → Identifier ✓ (not `if` + `1`)

---

#### Step E: Numeric Constant
```
IF isdigit(ch):
    num = ""
    WHILE current[i] is digit OR '.':
        num += current[i]
        i++

    print line, num, "Constant"
    continue
```

**Examples:** `42` → Constant, `3.14` → Constant, `20.5` → Constant
**Note:** We allow multiple dots (like `3.1.4`) — acceptable for an exam.

---

#### Step F: String Literal `"..."`
```
IF ch == '"':
    str = ""
    str += ch   ← include the opening quote
    i++

    WHILE current[i] != '"' AND current[i] != '$':
        str += current[i]
        i++

    IF current[i] == '"':  ← found the closing quote
        str += '"'
        i++

    print line, str, "Literal"
    continue
```

**Why stop at `$`?** In case the string is not closed before the buffer ends.
We don't want to read past valid memory. A real lexer would report "unterminated string".

---

#### Step G: Character Literal `'...'`
```
IF ch == '\'':
    Same logic as string literal but with single quote as delimiter
    e.g., 'A', '\n', '\t'
```

---

#### Step H: Operators with MAXIMAL MUNCH
```
IF ch is in {+, -, *, /, =, <, >, !, %}:
    op = ""
    op += ch        ← first character of operator

    IF ch is in {=, <, >, !} AND current[i+1] == '=':
        op += '='   ← TWO-CHAR OPERATOR: ==, <=, >=, !=
        i++         ← consume the second character NOW

    print line, op, "Operator"
    i++             ← consume the first character
    continue
```

**The Maximal Munch Decision Table:**

| We see | Peek at next | Result |
|--------|-------------|--------|
| `=` | `=` | `==` (relational) |
| `=` | `+` | `=` (assignment) |
| `<` | `=` | `<=` (relational) |
| `<` | `<` | `<` (just less-than, `<<` not handled here) |
| `!` | `=` | `!=` (relational) |
| `+` | anything | `+` (we don't handle `++` in this version) |

**Why `i++` at the end (outside the inner if)?**
When we enter this block, `i` still points to `ch` (the first operator char).
The `i++` inside the `if` consumes the SECOND char.
The `i++` outside (after the if) consumes the FIRST char.
So for `==`: inner `i++` consumes second `=`, outer `i++` consumes first `=`. Total: +2. ✓
For `=` alone: only outer `i++` fires. Total: +1. ✓

---

#### Step I: Delimiters
```
IF ch is in {;, ',', (, ), {, }, [, ]}:
    d = "" + ch

    print line, d, "Delimiter"
    i++
    continue
```

**Why separate from operators?** Convention. Delimiters structure code (group/separate),
operators compute. They also have different roles in grammar productions.

---

#### Step J: Unknown (fall-through)
```
i++     ← skip anything not matched above (e.g., @, #, ~)
```

---

## The `setw()` Formatting

```cpp
cout << left << setw(8) << line     // left-align, width 8
              << setw(20) << word   // left-align, width 20
              << "Keyword" << endl;
```

`left` — left-justifies text within the field width.
`setw(N)` — sets the MINIMUM column width. Text shorter than N is padded with spaces.

Output:
```
1       int                 Keyword
1       main                Identifier
```

---

## Full Sample Output for `input.txt`

Input:
```c
int main()
{
    int a = 10;
    float b = 20.5;
    char ch = 'A';
    // This is a comment
    if(a > 5)
    {
        a = a + 1;
        return a;
    }
    printf("Hello World");
}
```

Output:
```
Line    Lexeme              Token
---------------------------------------------------
1       int                 Keyword
1       main                Identifier
1       (                   Delimiter
1       )                   Delimiter
2       {                   Delimiter
3       int                 Keyword
3       a                   Identifier
3       =                   Operator
3       10                  Constant
3       ;                   Delimiter
4       float               Keyword
4       b                   Identifier
4       =                   Operator
4       20.5                Constant
4       ;                   Delimiter
5       char                Keyword
5       ch                  Identifier
5       =                   Operator
5       'A'                 Literal
5       ;                   Delimiter
7       if                  Keyword          ← line 6 skipped (comment)
...
```

---

## Key Decisions Summary

| Decision | Reason |
|----------|--------|
| Sentinel `$` not `\0` | Both work; `$` is printable, easier to debug |
| `'\n'` checked before `isspace()` | `isspace('\n')=true` would skip `\n` without counting lines |
| `//` skip loop doesn't do `i++` after | We want the next iteration to see `\n` and count the line |
| Identifier scan: `isalpha OR '_'` to START | Identifiers can't start with a digit (`1x` is invalid) |
| Identifier scan: `isalnum OR '_'` to CONTINUE | Digits allowed after first char (`x1` is valid) |
| Two-char operators: only `=`,`<`,`>`,`!` checked | Only these form two-char operators with `=` in our subset |
| Classify word AFTER collecting all chars | Maximal munch: avoid splitting `integer` into `int` + `eger` |

---

## How to Write This in Exam (Template)

```cpp
// TEMPLATE: que1.cpp (Two-buffer)
// 1. Global: buffer1[N+1], buffer2[N+1], forwardPtr, usingFirst
// 2. loadBuffer(): fin.read(buf, N); buf[gcount()] = '\0';
// 3. getChar(): read *fwd; if '\0' → switch buffer, reload; fwd++; return ch;
// 4. isKeyword(s): loop through keyword array, return true if match
// 5. main(): load both buffers → loop { c=getChar(); if isalnum → lexeme+=c; else → classify + clear; }

// TEMPLATE: que2.cpp (Full Lexer)
// 1. Outer loop: read chunk → set sentinel → inner loop
// 2. Inner loop priority order:
//    '\n' → line++  |  isspace → skip  |  '//' → skip line
//    isalpha/'_' → read word → keyword/identifier
//    isdigit → read number → constant
//    '"' → read until '"' → literal
//    '\'' → read until '\'' → literal
//    operator chars → maximal munch for ==, <=, >=, != → operator
//    delimiter chars → single char → delimiter
```
