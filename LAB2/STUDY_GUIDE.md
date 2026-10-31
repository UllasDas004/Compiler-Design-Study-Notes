# LAB 2 — Complete Study Guide
## Maximal Munch Lexical Analyzer & CFG Generation

---

## Files At a Glance

| File | Purpose | Focus |
|------|---------|-------|
| `que2.cpp` | Lexical Analyzer with **Maximal Munch** | Longest-match rule for operators, two-buffer system, full token classification |
| `que3.cpp` | **CFG Generator** for `aabaabaa` | Pattern identification, grammar construction, derivation steps |

---

# QUESTION 2 — Maximal Munch Lexical Analyzer (`que2.cpp`)

---

## What is Maximal Munch (Longest Match Rule)?

When a lexer sees certain characters, there can be **ambiguity** about how to tokenize them:

| Input | Could Be Tokenized As | Maximal Munch Chooses |
|-------|----------------------|----------------------|
| `==` | `=` + `=` (two assignment ops) | `==` (one relational op) |
| `>=` | `>` + `=` | `>=` (one relational op) |
| `<=` | `<` + `=` | `<=` (one relational op) |
| `!=` | `!` + `=` | `!=` (one relational op) |

**The Rule**: **Always consume the LONGEST valid token starting at the current position.**

> **Why it matters**: Without maximal munch, `a == b` becomes `IDENTIFIER, ASSIGN, ASSIGN, IDENTIFIER` — wrong! With it: `IDENTIFIER, RELATIONAL_OP, IDENTIFIER` — correct!

---

## How the Code Implements Maximal Munch

```cpp
// OPERATOR HANDLING WITH MAXIMAL MUNCH
if(ch=='+' || ch=='-' || ch=='*' || ch=='/' ||
   ch=='=' || ch=='<' || ch=='>' || ch=='!' ||
   ch=='%')
{
    string op;
    op += ch;   // Start with first char

    // MAXIMAL MUNCH: Check if NEXT char forms a 2-char operator
    if((ch=='=' || ch=='<' || ch=='>' || ch=='!') &&
       current[i+1]=='=')
    {
        op += '=';   // Append second '='
        i++;         // Consume second character NOW
    }

    print op as "Operator";
    i++;   // Consume first character
    continue;
}
```

### The Maximal Munch Decision Table

| We See | Peek Next | Result | Token Type |
|--------|-----------|--------|------------|
| `=` | `=` | `==` | RELATIONAL_OP |
| `=` | anything else | `=` | ASSIGNMENT_OP |
| `<` | `=` | `<=` | RELATIONAL_OP |
| `<` | anything else | `<` | RELATIONAL_OP |
| `>` | `=` | `>=` | RELATIONAL_OP |
| `>` | anything else | `>` | RELATIONAL_OP |
| `!` | `=` | `!=` | RELATIONAL_OP |
| `!` | anything else | `!` | OPERATOR |
| `+`, `-`, `*`, `/`, `%` | anything | single char | OPERATOR |

> **Note**: This implementation only handles `==`, `!=`, `<=`, `>=` as two-char operators. A full lexer would also handle `++`, `--`, `+=`, `-=`, `<<`, `>>`, `&&`, `||`, etc.

---

## Two-Buffer System (Review from LAB1)

This program uses the same two-buffer technique but with production-ready parameters:

| Feature | LAB1 `que1.cpp` | LAB2 `que2.cpp` |
|---------|-----------------|-----------------|
| Buffer size | 8 (demo) | **4096** (4KB, production) |
| Sentinel | `\0` | **`$`** (printable, easier to debug) |
| Token types | Keyword, Identifier | **ALL: Keyword, Identifier, Constant, Literal, Operator, Delimiter** |
| Maximal munch | ❌ Not implemented | ✅ **YES** for `==`, `!=`, `<=`, `>=` |
| Line tracking | ❌ No | ✅ **Yes** |
| Comments | ❌ Not handled | ✅ **`//` comments skipped** |
| Output format | Simple text | **Formatted table with `setw()`** |

### Why `$` Instead of `\0` as Sentinel?

Both work if they cannot appear in valid source code.
- `\0`: Naturally absent from text files, very safe
- `$`: Printable (easier to see while debugging)

**Key Rule**: The sentinel **MUST** be a character that **CANNOT** appear in valid source code.

---

## Algorithm Walkthrough — `que2.cpp`

### PHASE 1: Setup

```
1. Open "input.txt"
2. firstBuffer = true, line = 1
3. Print table header with setw()
```

### PHASE 2: Outer Loop (File Chunk Processing)

```
WHILE file has data:
    IF firstBuffer:  file.read(buffer1, BUFFER_SIZE)
    ELSE:            file.read(buffer2, BUFFER_SIZE)

    count = gcount()              [bytes actually read]
    IF count == 0: BREAK          [file exhausted]

    buffer[count] = '$'           [sentinel AFTER real data]
    current = (firstBuffer) ? buffer1 : buffer2
    i = 0

    INNER LOOP (process until '$')
    firstBuffer = !firstBuffer    [toggle for next chunk]
```

### PHASE 3: Inner Loop — Priority Order (First Match Wins)

The inner loop processes characters **sequentially in priority order**. The first matching `if` wins for each character.

#### Step A: Newline `\n`
```cpp
if(ch == '\n') {
    line++;    // MUST come before isspace()!
    i++;
    continue;
}
```
**WHY before `isspace()`?** Because `isspace('\n')` is `true`. If we just "skip whitespace", we'd skip `\n` **without** incrementing the line counter!

#### Step B: Whitespace (space, tab)
```cpp
if(isspace(ch)) { i++; continue; }
```
Not a token. Just skip.

#### Step C: Single-Line Comment `//`
```cpp
if(ch == '/' && current[i+1] == '/') {
    while(current[i] != '\n' && current[i] != '$')
        i++;
    continue;   // DON'T do i++ here! Leave i at '\n'
}
```
**Why check `current[i+1]`?** Because `/` alone is division operator. We only skip if BOTH are `/`.

**Why no `i++` after the loop?** We want the next iteration to see `\n` and count the line.

#### Step D: Identifier or Keyword
```cpp
if(isalpha(ch) || ch == '_') {
    string word;
    while(isalnum(current[i]) || current[i] == '_') {
        word += current[i];
        i++;
    }
    // Classify AFTER collecting full word (MAXIMAL MUNCH for identifiers)
    if(isKeyword(word)) print "Keyword" else print "Identifier";
    continue;
}
```

**Maximal Munch for Identifiers**: We greedily consume ALL alphanumeric chars.
- `int` → Keyword ✓
- `integer` → Identifier ✓ (NOT split into `int` + `eger`)
- `if1` → Identifier ✓ (NOT `if` + `1`)

**Why start with `isalpha || '_'` but scan with `isalnum`?**
- Identifiers **START** with letter or `_` (not digit)
- Identifiers **CONTAIN** letters, digits, `_` after first char
- `x1` valid, `1x` invalid (that's a number then error)

#### Step E: Numeric Constant
```cpp
if(isdigit(ch)) {
    string num;
    while(isdigit(current[i]) || current[i]=='.') {
        num += current[i];
        i++;
    }
    print "Constant";
    continue;
}
```
**Examples**: `42` → Constant, `3.14` → Constant
**Note**: Allows multiple dots (e.g., `3.1.4`) — acceptable for exam.

#### Step F: String Literal `"..."`
```cpp
if(ch == '"') {
    string str; str += ch; i++;
    while(current[i] != '"' && current[i] != '$') {
        str += current[i]; i++;
    }
    if(current[i] == '"') { str += '"'; i++; }
    print "Literal"; continue;
}
```
**Why stop at `$`?** In case string is not closed before buffer ends. Prevents reading garbage.

#### Step G: Character Literal `'...'`
```cpp
if(ch == '\'') {
    // Same logic as string literal with single quotes
}
```

#### Step H: Operators with MAXIMAL MUNCH
```cpp
if(ch is in {+, -, *, /, =, <, >, !, %}) {
    string op; op += ch;
    if((ch is =,<,>,!) && current[i+1] == '=') {
        op += '='; i++;   // consume second char
    }
    print "Operator"; i++; continue;  // i++ consumes first char
}
```

**The Two `i++` Logic**:
- At entry: `i` points to first operator char (`ch`)
- Inside `if`: `i++` consumes the **second** character (the `=`)
- After `if`: `i++` consumes the **first** character
- Result: `==` advances `i` by 2 ✓, `=` advances by 1 ✓

#### Step I: Delimiters
```cpp
if(ch in {; , ( ) { } [ ]}) {
    print "Delimiter"; i++; continue;
}
```
**Why separate from operators?** Convention. Delimiters structure code (group/separate), operators compute. Different roles in grammar productions.

#### Step J: Unknown (Fall-through)
```cpp
i++;  // skip @, #, ~, etc.
```

---

## The `setw()` Formatting Explained

```cpp
cout << left << setw(8) << line     // left-align, width 8
              << setw(20) << word   // left-align, width 20
              << "Keyword" << endl;
```

- `left` — left-justifies text within the field width
- `setw(N)` — sets **minimum** column width. Text shorter than N is padded with spaces.

**Output**:
```
Line    Lexeme              Token
---------------------------------------------------
1       int                 Keyword
1       main                Identifier
1       (                   Delimiter
1       )                   Delimiter
3       a                   Identifier
3       =                   Operator
3       10                  Constant
```

---

## Sample Run

**input.txt**:
```c
int a = 5;
if(a <= 5)
    a == b;
else
    a > b
```

**Output**:
```
Line    Lexeme              Token
---------------------------------------------------
1       int                 Keyword
1       a                   Identifier
1       =                   Operator
1       5                   Constant
1       ;                   Delimiter
2       if                  Keyword
2       (                   Delimiter
2       a                   Identifier
2       <=                  Operator
2       5                   Constant
2       )                   Delimiter
3       a                   Identifier
3       ==                  Operator
3       b                   Identifier
3       ;                   Delimiter
4       else                Keyword
5       a                   Identifier
5       >                   Operator
5       b                   Identifier
```

Note: `line 4` (the `else` line) has no tokens except the keyword. The `>` on line 5 is correctly tokenized as a single-char operator because next char is space, not `=`.

---

## Key Decisions Summary

| Decision | Reason |
|----------|--------|
| Sentinel `$` not `\0` | Both work; `$` is printable, easier to debug |
| `'\n'` checked before `isspace()` | `isspace('\n')=true` would skip `\n` without counting lines |
| `//` skip loop doesn't do `i++` after | Want next iteration to see `\n` and count the line |
| Identifier start: `isalpha \|\| '_'` | Identifiers can't start with digit (`1x` invalid) |
| Identifier continue: `isalnum \|\| '_'` | Digits allowed after first char (`x1` valid) |
| Two-char operators: only `=`,`<`,`>`,`!` | Only these form two-char ops with `=` in our subset |
| Classify word AFTER collecting all chars | Maximal munch: avoid splitting `integer` into `int`+`eger` |

---

## How to Write This in Exam (Template)

```cpp
// TEMPLATE: que2.cpp (Maximal Munch Lexer)
//
// 1. CONSTANTS & GLOBALS:
//    const int BUFFER_SIZE = 4096;
//    char buffer1[BUFFER_SIZE+1], buffer2[BUFFER_SIZE+1];
//
// 2. isKeyword(string):
//    string kw[] = {"int","float",...};
//    for each k: if(word==k) return true;
//    return false;
//
// 3. main():
//    ifstream file("input.txt");
//    bool firstBuffer = true; int line = 1;
//    print table header
//
//    while(true) {                    // OUTER: file chunks
//        read into buffer (alternating)
//        count = gcount(); if 0 break;
//        buffer[count] = '$';
//        current = firstBuffer ? buffer1 : buffer2;
//        i = 0;
//
//        while(current[i] != '$') {   // INNER: scan chars
//            ch = current[i];
//
//            // Priority order:
//            if(ch=='\n') { line++; i++; continue; }
//            if(isspace(ch)) { i++; continue; }
//            if(ch=='/' && current[i+1]=='/') { while(...) i++; continue; }
//
//            if(isalpha(ch) || ch=='_') {
//                word=""; while(isalnum||_) {word+=ch; i++;}
//                print keyword/identifier; continue;
//            }
//            if(isdigit(ch)) {
//                num=""; while(isdigit||'.') {num+=ch; i++;}
//                print Constant; continue;
//            }
//            if(ch=='"' || ch=='\'') { ... read literal ... continue; }
//
//            // MAXIMAL MUNCH OPERATORS:
//            if(ch in {+,-,*,/,=,<,>,!,%}) {
//                op=ch;
//                if((ch in {=,<,>,!}) && current[i+1]=='=') {op+='='; i++;}
//                print Operator; i++; continue;
//            }
//
//            if(ch in {;,,,(,),{,},[,]}) {
//                print Delimiter; i++; continue;
//            }
//
//            i++;  // unknown
//        }
//        firstBuffer = !firstBuffer;
//    }
```

---

# QUESTION 3 — CFG Generation for `aabaabaa` (`que3.cpp`)

---

## What is a Context-Free Grammar (CFG)?

A **CFG** is a set of production rules describing a formal language.
- **Terminals**: Actual symbols of the language (`a`, `b`)
- **Non-terminals**: Variables that get replaced (`S`, `A`, `B`)
- **Productions**: Rules like `S -> aabS | aa`
- **Start Symbol**: Where derivation begins (usually `S`)

---

## Analyzing the String `aabaabaa`

```
aabaabaa
^^^        = "aab"  (first block)
   ^^^     = "aab"  (second block)
      ^^   = "aa"   (tail)
```

**Pattern**: `(aab)` repeated `n` times, followed by `aa`
**Formula**: `L = { (aab)^n aa | n > 0 }`

For this string: `n = 2`

---

## The CFG

```
S -> aabS | aa
```

**Meaning**:
- `S -> aabS`: Add one more "aab" block, recurse (for n > 1)
- `S -> aa`: Base case, terminate with the tail "aa"

This is a **right-linear grammar** (non-terminal only at right end). Right-linear grammars generate **regular languages** — and indeed this language IS regular (regex: `(aab)+aa`).

---

## Derivation for n=2 (the given string)

```
S
=> aabS          (apply S -> aabS)
=> aabaabS       (apply S -> aabS again)
=> aabaabaa      (apply S -> aa)
```

Each `=>` represents **one production application**.

---

## Sample Output from the Code

```
Analyzing string: aabaabaa
--------------------------------------------------
1. Language Pattern:
   The string consists of the repeating block 'aab' followed by 'aa'.
   Pattern formula: L = { (aab)^n aa | n > 0 }
   For the given input 'aabaabaa', n = 2.

2. Context-Free Grammar (CFG):
   S -> aabS | aa

3. Derivation for n = 2:
   S
   => aabS
   => aabaabS
   => aabaabaa (Final derived string)
```

---

## General Solution Template for CFG Questions

Given ANY string, follow these steps:

### Step 1: Identify the Pattern
Write the string and group to find repeating unit + tail:
```
abbabbabbc
^^^ ^^^ ^^^ ^
abb abb abb c
```
- Repeating unit: `"abb"`
- Tail: `"c"`
- Count n: 3

### Step 2: Language Formula
`L = { (unit)^n tail | n > 0 }`
`L = { (abb)^n c | n > 0 }`

### Step 3: CFG
`S -> unit S | tail`
`S -> abb S | c`

### Step 4: Derivation (for the given n)
```
S
=> abbS
=> abbabbS
=> abbabbabbS
=> abbabbabbc
```

### Step 5: Verify
- Does the final string match input? ✓
- Is n > 0? ✓

---

## Regular vs Context-Free Languages

| Type | Description | Example |
|------|-------------|---------|
| **Regular** | Described by regex, DFA/NFA | `a*b`, `[0-9]+`, `(aab)+aa` |
| **Context-Free** | Described by CFG, push-down automaton | `a^n b^n`, balanced parentheses |

The language `{ (aab)^n aa | n > 0 }` is **regular** but writing it as a CFG is the systematic way to derive it.

> **Exam Tip**: Even if the language is regular, the question asks for CFG. The pattern `S -> unit S | tail` is the standard answer for "repeating unit + tail" strings.

---

## Key Theory to Remember

### Maximal Munch
- **Rule**: Longest valid token starting at current position
- **Implementation**: Peek next character before deciding
- **Applies to**: Operators (`==`, `>=`, etc.), identifiers (`integer` not `int`+`eger`), numbers

### CFG Construction for Pattern Strings
- **Pattern**: `(repeating_unit)^n tail` where `n > 0`
- **Grammar**: `S -> repeating_unit S | tail`
- **Derivation**: Apply recursive rule `n` times, then base case once

### Two-Buffer System
- **Purpose**: Efficient I/O — read chunks, not single chars
- **Mechanism**: Ping-pong between two buffers
- **Sentinel**: Special char marking end of valid data in buffer
- **Key insight**: Switch to pre-loaded buffer INSTANTLY, then refill the consumed one

---

## Quick Reference Card

```
MAXIMAL MUNCH OPERATORS:
=======================
See '=' -> peek next -> if '=' consume both (==)
See '<' -> peek next -> if '=' consume both (<=)
See '>' -> peek next -> if '=' consume both (>=)
See '!' -> peek next -> if '=' consume both (!=)

CFG PATTERN:
============
String: unit unit unit ... unit tail
        ^^^^^^  n times  ^^^^
CFG:    S -> unit S | tail

DERIVATION:
===========
S
=> unit S
=> unit unit S
=> ... (n times)
=> unit^n tail

TWO-BUFFER SENTINEL:
====================
buffer[count] = sentinel  // count = gcount(), NOT BUFFER_SIZE!
// This handles partial reads at EOF correctly.
```

---

## Common Exam Mistakes to Avoid

| Mistake | Correct Approach |
|---------|------------------|
| Checking `isspace()` before `'\n'` | Check `'\n'` FIRST, increment line, then `continue` |
| Classifying keyword mid-scan | Collect FULL word first, THEN classify (maximal munch) |
| Forgetting `continue` after token | Each branch MUST `continue` to avoid double `i++` |
| Sentinel at `BUFFER_SIZE` | Sentinel at `count` (actual bytes read) |
| Not handling `//` comment correctly | Skip to `\n` or `$`, DON'T consume `\n`, let outer loop count it |
| CFG: writing `S -> S unit | tail` | Must be RIGHT-linear: `S -> unit S | tail` |
| CFG: allowing n=0 | Question says `n > 0`, so base case is `tail`, not empty |

---

## Practice Variations

### Variation 1: Different Operators
If question asks for `++`, `--`, `+=`, `-=`:
```cpp
if(ch == '+' && current[i+1] == '+') { op += '+'; i++; }
if(ch == '-' && current[i+1] == '-') { op += '-'; i++; }
if((ch == '+' || ch == '-') && current[i+1] == '=') { op += '='; i++; }
```

### Variation 2: Different String Pattern
Input: `abcabcab`
- Unit: `abc`, Tail: `ab`, n=2
- CFG: `S -> abcS | ab`

### Variation 3: Multi-line Comments `/* */`
```cpp
if(ch == '/' && current[i+1] == '*') {
    i += 2;
    while(!(current[i] == '*' && current[i+1] == '/') && current[i] != '$')
        i++;
    if(current[i] != '$') i += 2;  // consume */
    continue;
}
```

---

## Final Checklist Before Exam

- [ ] Can write two-buffer system from memory
- [ ] Can implement maximal munch for `==`, `!=`, `<=`, `>=`
- [ ] Know why `'\n'` handled before `isspace()`
- [ ] Know why `//` comment skip doesn't consume `\n`
- [ ] Can identify pattern unit + tail from any string
- [ ] Can write CFG `S -> unit S | tail`
- [ ] Can show derivation steps with `=>` arrows
- [ ] Know difference between regular and context-free languages
- [ ] Can format output table with `setw()` and `left`