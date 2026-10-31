# LAB 8 — Three-Address Code (TAC) Generation

## Question

Write a program that accepts an arithmetic expression or assignment statement (e.g., `a = b + c * d`):
- (a) Performs lexical analysis and parsing (recursive descent parser)
- (b) Generates Three-Address Code (TAC)
- (c) Converts TAC into Quadruple and Triple representations
- (d) Handles: `+`, `-`, `*`, `/`, `=`
- (e) Uses temporaries: `t1`, `t2`, ...

**File:** `que_tac.cpp`

---

## Big Picture: What is Three-Address Code?

TAC is an **intermediate representation** — it's what sits between your high-level source code
and machine code. It's simple: each instruction has at most **three addresses** (two operands + result).

```
High-level:   a = b + c * d

TAC:          t1 = c * d          <- evaluate c*d first (higher precedence)
              t2 = b + t1         <- then add b
              a = t2              <- assign result to a
```

TAC is like "machine code for a virtual machine" — easy to optimize and translate to real assembly.

---

## The Three Output Formats

### 1. Three-Address Code (TAC)
```
t1 = c * d
t2 = b + t1
a = t2
```
Human-readable, explicit temporary variables.

### 2. Quadruples — (Op, Arg1, Arg2, Result)
```
#     Op        Arg1      Arg2      Result
----------------------------------------------
0     *         c         d         t1
1     +         b         t1        t2
2     =         t2        -         a
```
Four fields per instruction. Result field names the temporary explicitly.
**Good for optimization** — can reorder instructions freely.

### 3. Triples — (Index, Op, Arg1, Arg2)
```
Index   Op        Arg1      Arg2
------------------------------------------
(0)     *         c         d
(1)     +         b         (0)       <- "(0)" means "use result of instruction 0"
(2)     =         (1)       a (Target)
```
No Result field — the index IS the result reference.
**Saves space** but harder to optimize (moving instruction 0 would break index (0) references).

---

## Code Architecture

```
main()
  |
  |-- Lexer.tokenize()    -> vector<Token>
  |
  |-- Parser.parseStatement()
  |     |
  |     |-- is it "ID = ..."?  YES -> skip ID and =, then parseExpression()
  |     |                              then emit("=", result, "", target)
  |     |
  |     +-- NO -> just parseExpression()
  |
  |-- Parser.printTAC()
  |-- Parser.printQuadruples()
  +-- Parser.printTriples()
```

---

## The Lexer

Breaks the input string into tokens:

```cpp
enum TokenType { ID, NUM, OP, ASSIGN, LPAREN, RPAREN, END };
struct Token { TokenType type; string value; };

// "a = b + c * d" becomes:
// [ID,"a"] [ASSIGN,"="] [ID,"b"] [OP,"+"] [ID,"c"] [OP,"*"] [ID,"d"]
```

Key: `=` is a separate token type (ASSIGN), distinct from arithmetic operators (OP).
This lets the parser easily detect `ID = expression` assignment patterns.

---

## The Parser — Recursive Descent with TAC Emission

### Grammar
```
Statement  -> ID = Expression  |  Expression
Expression -> Term ((+ | -) Term)*      [handles +, -, lower precedence]
Term       -> Factor ((* | /) Factor)*  [handles *, /, higher precedence]
Factor     -> '(' Expression ')' | ID | NUM
```

### How Operator Precedence Works

`parseTerm()` calls `parseFactor()` and handles `*` and `/`.
`parseExpression()` calls `parseTerm()` and handles `+` and `-`.

Since `parseTerm()` is called INSIDE `parseExpression()`, it runs first.
So `*` and `/` are always evaluated before `+` and `-`.

### parseFactor() — Base Case
```cpp
string parseFactor() {
    if (token is '('):
        consume '('
        result = parseExpression()   // recurse for the inner expression
        consume ')'
        return result

    if (token is ID or NUM):
        return the token's value string (e.g., "b", "42")
}
```

### parseTerm() — Handles * and /
```cpp
string parseTerm() {
    left = parseFactor()    // get the first operand

    while (next token is '*' or '/'):
        op = consume the operator
        right = parseFactor()
        temp = newTemp()          // generate t1, t2, ...
        emit(op, left, right, temp)  // TAC: temp = left op right
        left = temp               // temp becomes the new left
    
    return left
}
```

### parseExpression() — Handles + and -
```cpp
string parseExpression() {
    left = parseTerm()      // get the first term (already handles * and /)

    while (next token is '+' or '-'):
        op = consume the operator
        right = parseTerm()
        temp = newTemp()
        emit(op, left, right, temp)
        left = temp
    
    return left
}
```

### parseStatement() — Handles Assignment
```cpp
void parseStatement() {
    if (first token is ID and second is ASSIGN):
        target = tokens[0].value   // e.g., "a"
        pos = 2                    // skip past "a" and "="
        result = parseExpression() // parse the RHS
        emit("=", result, "", target)  // TAC: a = t2
    else:
        parseExpression()          // just evaluate, no assignment
}
```

---

## Working Through `a = b + c * d` Step by Step

**Tokens:** `a` `=` `b` `+` `c` `*` `d`

1. `parseStatement()` detects assignment. target = `"a"`, pos jumps to `b`
2. `parseExpression()` calls `parseTerm()`
3. `parseTerm()` calls `parseFactor()` → returns `"b"` (no `*` or `/` follows immediately? Wait...)
   - Actually parseTerm peeks: next token is `+`, not `*` or `/`, so it returns `"b"` immediately
4. Back in `parseExpression()`: sees `+`, advances, calls `parseTerm()` for right
5. `parseTerm()` calls `parseFactor()` → returns `"c"`
6. `parseTerm()` peeks: sees `*`, advances, calls `parseFactor()` → returns `"d"`
7. `parseTerm()` emits: **`t1 = c * d`** → returns `"t1"`
8. `parseExpression()` emits: **`t2 = b + t1`** → returns `"t2"`
9. `parseStatement()` emits: **`a = t2`**

**Result:**
```
t1 = c * d
t2 = b + t1
a = t2
```

---

## newTemp() and emit()

```cpp
string newTemp() {
    return "t" + to_string(tempCount++);  // t1, t2, t3, ...
    // tempCount starts at 1
}

void emit(string op, string arg1, string arg2, string result) {
    tacList.push_back({op, arg1, arg2, result});
    // Adds a TAC struct to the list
}
```

The `tacList` is a `vector<TAC>` that accumulates all generated instructions.

---

## Triples: How Index References Work

```cpp
// Build map: temp name -> which instruction computed it
map<string, int> tempToIndex;
for (int i = 0; i < tacList.size(); i++)
    if (tacList[i].result[0] == 't')     // result starts with 't' = it's a temp
        tempToIndex[tacList[i].result] = i;

// When printing, replace temp names with their index
// e.g., "t1" -> "(0)" if t1 was computed in instruction 0
```

---

## Sample Outputs

### Input: `a = b + c * d`
```
=== Input: a = b + c * d ===

--- Tokens ---
[ID, a]  [ASSIGN, =]  [ID, b]  [OP, +]  [ID, c]  [OP, *]  [ID, d]

--- Three-Address Code (TAC) ---
t1 = c * d
t2 = b + t1
a = t2

--- Quadruples ---
#     Op        Arg1      Arg2      Result
---------------------------------------------
0     *         c         d         t1
1     +         b         t1        t2
2     =         t2        -         a

--- Triples ---
Index   Op        Arg1        Arg2
------------------------------------------
(0)     *         c           d
(1)     +         b           (0)
(2)     =         (1)         a (Target)
```

### Input: `(a + b) * c`
```
--- Three-Address Code (TAC) ---
t1 = a + b
t2 = t1 * c

--- Quadruples ---
0     +         a         b         t1
1     *         t1        c         t2

--- Triples ---
(0)     +         a           b
(1)     *         (0)         c
```

### Input: `x = a - b * c + d`
```
--- Three-Address Code (TAC) ---
t1 = b * c        <- * is highest precedence
t2 = a - t1       <- left-to-right for same precedence
t3 = t2 + d
x = t3
```

---

## Comparison: Quadruples vs Triples

| Feature | Quadruples | Triples |
|---------|-----------|---------|
| Fields | 4: op, arg1, arg2, result | 3: op, arg1, arg2 |
| Result stored | Explicitly named (t1, t2) | By index reference (0), (1) |
| Instruction reordering | Easy (name-based) | Hard (index-based, breaks refs) |
| Space usage | More (stores temp names) | Less (just indices) |
| Used in practice | More common | Less common |

---

## Compile & Run

```bash
g++ -o que_tac.exe que_tac.cpp

# Test cases:
echo "a = b + c * d"    | .\que_tac.exe
echo "(a + b) * c"      | .\que_tac.exe
echo "x = a - b * c + d" | .\que_tac.exe
echo "a + b"            | .\que_tac.exe     # no assignment
echo "5 + 3 * 2"        | .\que_tac.exe     # numbers as operands
```

---

## Exam Tips

- Temporaries are generated **bottom-up**: inner operations first (higher precedence)
- `emit()` stores instructions; printing happens AFTER all parsing is done
- Quadruple: four explicit fields, result IS the temp name
- Triple: no result field; instead use the instruction index `(0)`, `(1)`, etc.
- Assignment: `a = t2` is emitted as `("=", "t2", "", "a")`
  - op="=", arg1=t2, arg2="" (empty), result=a
- Parentheses force `parseExpression()` to be called recursively, handling `(a+b)*c` correctly
