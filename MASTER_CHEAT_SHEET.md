# Compiler Design — Master Exam Cheat Sheet
## Quick Reference for All 8 Labs

---

## LAB 1 — Two-Buffer System & Lexical Analyzer

### Two-Buffer System (que1.cpp)
```
Buffer 1: [chars...\0]   Buffer 2: [chars...\0]
             ^sentinal              ^sentinel
forwardPtr moves right; hits \0 = switch to other buffer
```
- `loadBuffer(buf)` = reads BUFFER_SIZE chars from file into buf, puts `\0` at end
- `getChar()` = returns current char, advances ptr, switches buffer at `\0`
- Classify: `isalnum(c)||c=='_'` -> identifier; else -> end of token -> check if keyword

### Full Lexer (que2.cpp)
- Sentinel: `$` at `buffer[count]` after file read
- Track `line` number (increment on `\n`)
- **Maximal Munch**: `=` -> check if `==`; `>` -> check if `>=`; etc.
- Token types: Keyword, Identifier, Constant, Operator, Delimiter, Literal

---

## LAB 2 — Maximal Munch & CFG

### Maximal Munch
```cpp
if (input[i] == '=') {
    if (i+1 < n && input[i+1] == '=') { /* == */ i += 2; }
    else                               { /* =  */ i++;   }
}
```

### CFG for `aabaabaa`
```
Pattern: (aab)^n aa  where n > 0
Grammar: S -> aabS | aa
Derivation: S => aabS => aabaabS => aabaabaa
```

---

## LAB 3 — Recursive Descent Parser + AST

### Grammar
```
E  -> T E'        E' -> +TE' | -TE' | eps
T  -> F T'        T' -> *FT' | /FT' | eps
F  -> (E) | id | num
```

### Parser Functions
```cpp
Node* E() {                    // handles + and -
    left = T();
    while (match('+')) { right = T(); left = new Node("OP","+",left,right); }
    return left;
}
Node* T() { /* same pattern with * and / */ }
Node* F() {
    if(match('(')) { node = E(); match(')'); return node; }
    if(substr=="id")  return new Node("ID","id");
    if(substr=="num") return new Node("NUM","num");
}
```

---

## LAB 4 — LL(1) Parsing + Left Recursion Elimination

### Grammar: `S->aABe, A->bAc|d, B->f|eps`

| | FIRST | FOLLOW |
|--|-------|--------|
| S | {a} | {$} |
| A | {b,d} | {f,e} |
| B | {f,eps} | {e} |

### Parsing Algorithm
```
Stack: [$, S]    Input: adfe$

if top == input == $:   ACCEPT
if top == input:        match -> pop stack, advance input
if M[top][input] exists: expand -> pop, push production reversed
else: ERROR
```

### Left Recursion Elimination
```
E -> E + T | T     becomes:    E  -> T E'
                               E' -> + T E' | eps
```
Rule: `A -> Aalpha | beta` converts to `A -> betaA'` and `A' -> alphaA' | eps`

---

## LAB 5 — LR(0) Parser

### Grammar: `S'->S, S->A, A->Ab|a`

| State | a | b | $ | S | A |
|-------|---|---|---|---|---|
| 0 | s3 | | | 1 | 2 |
| 1 | | | Acc | | |
| 2 | r1 | s4 | r1 | | |
| 3 | r3 | r3 | r3 | | |
| 4 | r2 | r2 | r2 | | |

- **Shift**: terminal after dot -> push token + new state
- **Reduce r1**: dot at end -> pop |RHS| states, push GOTO[top][LHS]
- **Accept**: S'->S. with `$`
- **Closure**: for `[A -> . B beta]`, add all `[B -> . gamma]`
- **GOTO(I, X)**: move dot past X, then take closure

---

## LAB 6 — Flex & Bison

### Flex (.l file) Structure
```
%{ C headers %}
%%
regex_pattern  { action; return TOKEN_TYPE; }
%%
int yywrap() { return 1; }
```

### Bison (.y file) Key Parts
```
%union { int num; char* str; }
%token <str> IDENTIFIER      // token with string value
%token <num> NUMBER          // token with int value
%type  <num> expr            // non-terminal with int value
%left '+' '-'                // low precedence, left-associative
%left '*' '/'                // high precedence
%%
grammar rules:
expr: NUMBER         { $$ = $1; }              // $$ = result, $1 = first symbol
    | expr '+' expr  { $$ = $1 + $3; }         // $3 = third symbol
    | '(' expr ')'   { $$ = $2; }              // $2 = middle (the expr)
```

### Build Commands
```bash
flex config.l
bison -d config.y        # -d creates .tab.h (needed by flex)
gcc lex.yy.c config.tab.c -o program -lfl
./program
```

### Key: The dollar notation
- `$1`, `$2`, `$3` = values of 1st, 2nd, 3rd symbol on the RHS
- `$$` = value you assign to the LHS (the production's result)
- `yylval.str` / `yylval.num` = how flex passes values to bison

---

## LAB 7 — Semantic Analysis & Type Checking

### Symbol Table
```cpp
map<string, DataType> symbolTable;   // {"x": INT_T, "y": FLOAT_T}
```

### analyze() Logic
```
DECL    -> check redeclaration; symbolTable[name] = type
ID      -> check undeclared; return symbolTable[name]
CONST   -> return node->dataType
BINOP   -> analyze children; if types differ -> TYPE MISMATCH
ASSIGN  -> analyze both sides; if types differ -> TYPE MISMATCH
```

### Error Types
| Error | Trigger |
|-------|---------|
| Redeclaration | Same variable declared twice |
| Undeclared | Used before declared |
| Type mismatch (BINOP) | `int + float` |
| Type mismatch (ASSIGN) | `int x; x = 3.14;` |

---

## LAB 8 — Three-Address Code (TAC) Generation

### Grammar (recursive descent)
```
Statement  -> ID = Expression  |  Expression
Expression -> Term ((+ | -) Term)*       [lower precedence]
Term       -> Factor ((* | /) Factor)*   [higher precedence]
Factor     -> '(' Expression ')' | ID | NUM
```

### Key Pattern: Emit TAC During Parsing
```cpp
string parseTerm() {
    left = parseFactor();
    while (next is * or /):
        op = consume operator
        right = parseFactor()
        temp = newTemp()             // "t1", "t2", ...
        emit(op, left, right, temp)  // t1 = left op right
        left = temp
    return left
}
```

### Three Representations for `a = b + c * d`

**TAC:**
```
t1 = c * d
t2 = b + t1
a = t2
```

**Quadruples (op, arg1, arg2, result):**
```
#  Op   Arg1  Arg2  Result
0  *    c     d     t1
1  +    b     t1    t2
2  =    t2    -     a
```

**Triples (index, op, arg1, arg2) -- no result field, use index to refer:**
```
(0)  *   c    d
(1)  +   b    (0)    <- (0) means "result of instruction 0"
(2)  =   (1)  a [Target]
```

---

## The Compiler Pipeline (Big Picture)

```
Source Code
    |
    v
[LAB 1-2]  Lexical Analysis    tokens (keywords, IDs, numbers, operators)
    |
    v
[LAB 3-5]  Parsing             AST / parse tree (is the syntax correct?)
    |
    v
[LAB 6]    Flex + Bison        automated tool to generate lexer + parser
    |
    v
[LAB 7]    Semantic Analysis   type checking, symbol table (is the meaning correct?)
    |
    v
[LAB 8]    Code Generation     Three-Address Code (TAC) for intermediate code
    |
    v
   Machine Code
```
