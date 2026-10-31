# LAB 3 — Complete Study Guide
## Recursive Descent Parser, AST Construction & Flex Lexer

---

## Files At a Glance

| File | Purpose | Focus |
|------|---------|-------|
| `que1.cpp` | **Struct-based** recursive descent parser | Direct string parsing, simple Node struct, iterative E'/T' |
| `temp.cpp` | **OOP-style** parser with separate Lexer | Token stream, polymorphic AST, recursive E'/T', RAII |
| `que2.l` | **Flex lexer specification** | Regex-based tokenization, maximal munch automatic |

---

# THE GRAMMAR (Left-Recursion-Free)

```
E  -> T E'
E' -> + T E' | - T E' | epsilon
T  -> F T'
T' -> * F T' | / F T' | epsilon
F  -> ( E ) | id | num
```

## Why This Grammar?

**Direct left-recursion** breaks recursive descent parsers:
```
E -> E + T | T          // E() calls E() calls E()... INFINITE RECURSION!
```

**Left recursion elimination** using E' (E-prime):
```
E  -> T E'              // Parse Term first, then handle rest
E' -> + T E' | - T E' | epsilon   // Loop for more +/-
```

This is **LL(1)** - suitable for recursive descent with 1-token lookahead.

---

## Precedence Via Call Hierarchy

```
E() calls T() calls F()
```

| Function | Handles | Precedence |
|----------|---------|------------|
| `E()` | `+`, `-` | Lowest (1) |
| `T()` | `*`, `/` | Higher (2) |
| `F()` | `id`, `num`, `(E)` | Highest (3) |

**Lower in call stack = higher precedence = evaluated first**

Example: `id + id * num`
```
E()                          <- handles +
  T()                        <- first id
  sees '+', calls T()        <- second id
    F() -> id
    sees '*', calls F()      <- num
    builds * node
  builds + node
```
Tree structure: `+` at root, `*` as right child → `*` evaluated first!

---

## Associativity

Both `E'` and `T'` produce **LEFT-ASSOCIATIVE** trees:

```
id - id - id    =>    (id - id) - id
id / id / id    =>    (id / id) / id
```

Because the previous result becomes the **left child** of the new operator node.

---

# QUESTION 1 — Struct-Based Parser (`que1.cpp`)

## Data Structure

```cpp
struct Node {
    string type;   // "OP", "ID", "NUM"
    string value;  // "+", "-", "*", "/", "id", "num"
    Node* left;
    Node* right;
};
```

Simple but effective. No polymorphism - uses `type` field to distinguish.

---

## Key Functions

### `match(char c)` - Core Primitive
```cpp
bool match(char c) {
    skipSpaces();
    if (pos < input.length() && input[pos] == c) {
        pos++; return true;
    }
    return false;
}
```
**Pattern**: skipSpaces → check → advance if match → return bool

---

### `E()` - Expression (handles +, -)
```cpp
Node* E() {
    Node* left = T();           // Parse first term

    while (true) {              // Loop for E' -> + T E' | - T E' | epsilon
        if (match('+')) {
            Node* right = T();
            left = new Node("OP", "+", left, right);
        } else if (match('-')) {
            Node* right = T();
            left = new Node("OP", "-", left, right);
        } else break;
    }
    return left;
}
```
- Calls `T()` first (precedence!)
- While loop = iterative implementation of right-recursive E'
- Each iteration: `left = new Node(op, left, right)` → LEFT-ASSOCIATIVE

---

### `T()` - Term (handles *, /)
```cpp
Node* T() {
    Node* left = F();           // Parse first factor

    while (true) {              // Loop for T' -> * F T' | / F T' | epsilon
        if (match('*')) {
            Node* right = F();
            left = new Node("OP", "*", left, right);
        } else if (match('/')) { ... }
        else break;
    }
    return left;
}
```
- Same pattern as `E()` but for higher-precedence operators
- Called by `E()`, so `*` binds tighter than `+`

---

### `F()` - Factor (base case)
```cpp
Node* F() {
    skipSpaces();
    if (match('(')) {
        Node* node = E();       // Recursive call!
        if (!match(')')) error;
        return node;
    }
    if (input.substr(pos,2)=="id") { pos+=2; return new Node("ID","id"); }
    if (input.substr(pos,3)=="num") { pos+=3; return new Node("NUM","num"); }
    error;
}
```
- Handles parentheses via recursive call to `E()`
- Direct string matching (no separate lexer)
- Base of recursion - no further calls to T/E

---

## Evaluation

```cpp
int evaluate(Node* root) {
    if (root->type == "ID")  return values["id"];  // 10
    if (root->type == "NUM") return values["num"]; // 5

    int left = evaluate(root->left);
    int right = evaluate(root->right);

    if (root->value == "+") return left + right;
    // ... -, *, /
}
```
**Post-order traversal**: children evaluated before parent → precedence respected automatically.

---

## Sample Output

```
Expression: id + id * num
AST:
+
  id
  *
    id
    num
Value: 60       (10 + 10*5 = 60)

Expression: (id - num) / id
AST:
/
  -
    id
    num
  id
Value: 0        ((10-5)/10 = 0 in integer division)
```

---

# QUESTION 1 (Advanced) — OOP Parser (`temp.cpp`)

## Architecture

```
Lexer (string → Token stream)
    ↓
Parser (Token stream → AST)
    ↓
AST (Polymorphic nodes with virtual evaluate/print)
```

---

## Token System

```cpp
enum TokenType { ID, NUM, PLUS, MINUS, MUL, DIV, LPAREN, RPAREN, END };
struct Token { TokenType type; string value; };
```

Lexer produces tokens, Parser consumes them. Clean separation!

---

## Lexer Class

```cpp
class Lexer {
    string text; size_t pos;
    Token getNextToken() {
        skip whitespace
        if alpha: read identifier → ID or NUM (for "num")
        if digit: read number → NUM
        else: single char → PLUS, MINUS, etc.
    }
};
```

---

## Polymorphic AST

```cpp
class ASTNode { virtual int evaluate(env) = 0; virtual print() = 0; };
class IdNode    : ASTNode { string name; int evaluate() { return env[name]; } };
class NumNode   : ASTNode { string val;  int evaluate() { return stoi(val); } };
class BinaryOpNode : ASTNode { char op; ASTNode *L,*R; int evaluate() { apply op; } };
```

**Benefits**:
- Each node type knows how to evaluate itself (no switch on type!)
- Virtual destructor deletes children (RAII - no memory leaks)
- Easy to extend: add `UnaryOpNode`, `AssignNode`, etc.

---

## Parser Class

```cpp
class Parser {
    Lexer lexer;
    Token currentToken;   // 1-token lookahead

    void advance() { currentToken = lexer.getNextToken(); }
    bool match(Type t) { if (currentToken.type==t) { advance(); return true; } return false; }
    void expect(Type t) { if (!match(t)) throw error; }
};
```

---

## Grammar Methods (Recursive E'/T')

```cpp
// E -> T E'
ASTNode* parseE() {
    ASTNode* left = parseT();
    return parseEPrime(left);
}

// E' -> + T E' | - T E' | epsilon
ASTNode* parseEPrime(ASTNode* left) {
    if (currentToken.type == PLUS) {
        advance();
        ASTNode* right = parseT();
        return parseEPrime(new BinaryOpNode('+', left, right));  // RECURSIVE!
    } else if (currentToken.type == MINUS) { ... }
    return left;  // epsilon
}
```

**Recursive E'** directly mirrors the grammar rule `E' -> + T E'`.
**Iterative E'** (que1.cpp) uses `while(true)` loop.
**Both produce identical left-associative trees.**

---

## Memory Management

```cpp
class BinaryOpNode {
    ~BinaryOpNode() { delete left; delete right; }  // RAII!
};
```
- Tree owns its children
- `delete root` recursively deletes entire tree
- No manual cleanup needed in main

---

## Error Handling

```cpp
try {
    Parser parser(expr);
    ASTNode* ast = parser.parse();
    ast->evaluate(env);
    delete ast;
} catch (const exception& e) {
    cout << "Error: " << e.what() << "\n";
}
```
- Exceptions instead of `exit(1)`
- Can recover, report multiple errors, etc.

---

# QUESTION 2 — Flex Lexer (`que2.l`)

## Lex File Structure

```
%{
  C code (includes, globals)
%}

DEFINITIONS:
  NAME    REGEX

%%
RULES:
  REGEX   { ACTION; }
%%
USER CODE:
  int yywrap() { return 1; }
  int main() { yylex(); }
```

---

## Rules (Order Matters!)

```lex
"int"|"float"|"if"|"else"|"return" { printf("KEYWORD: %s\n", yytext); }
{ID}        { printf("IDENTIFIER: %s\n", yytext); }
{DIGIT}+    { printf("NUMBER (INTEGER): %s\n", yytext); }
"+"|"-"|"*"|"/" { printf("ARITHMETIC OPERATOR: %s\n", yytext); }
"="         { printf("ASSIGNMENT OPERATOR: %s\n", yytext); }
"=="|"!="   { printf("RELATIONAL OPERATOR: %s\n", yytext); }
";"|","     { printf("PUNCTUATOR: %s\n", yytext); }
[ \t\n]+    { /* ignore */ }
.           { printf("UNKNOWN TOKEN: %s\n", yytext); }
```

**Critical Rule Order**:
1. **Keywords** BEFORE identifiers (same length, first wins)
2. **Longer operators** (`==`, `!=`) before shorter (`=`, `!`)
3. **Specific** before **general** (ID, NUMBER)
4. **Catch-all** (`.`) **LAST**

---

## Maximal Munch in Flex

**Automatic!** Flex always picks the **longest match**.
```
Input: "=="
Rules: "=="  { ... }    "="  { ... }
Match: "==" (2 chars) - longest wins!
```
No manual peeking needed like in hand-written lexer.

---

## Compilation

```bash
flex que2.l           # Generates lex.yy.c
gcc lex.yy.c -o lexer -lfl   # -lfl links flex library
./lexer
# Type input, Ctrl+D to end
```

---

## Sample Run

Input:
```c
int x = 10;
float y = x + 5;
if (x != y) return 0;
```

Output:
```
KEYWORD: int
IDENTIFIER: x
ASSIGNMENT OPERATOR: =
NUMBER (INTEGER): 10
PUNCTUATOR: ;
KEYWORD: float
IDENTIFIER: y
ASSIGNMENT OPERATOR: =
IDENTIFIER: x
ARITHMETIC OPERATOR: +
NUMBER (INTEGER): 5
PUNCTUATOR: ;
KEYWORD: if
PUNCTUATOR: (
IDENTIFIER: x
RELATIONAL OPERATOR: !=
IDENTIFIER: y
PUNCTUATOR: )
KEYWORD: return
NUMBER (INTEGER): 0
PUNCTUATOR: ;
```

---

# EXAM TEMPLATES

## Template 1: Struct-Based Parser (que1.cpp style)

```cpp
struct Node { string type, value; Node *L, *R; };
string input; int pos;
bool match(char c) { skipSpaces(); if (input[pos]==c) { pos++; return 1; } return 0; }

Node* E() { Node* L=T(); while(1){ if(match('+')) L=new Node("OP","+",L,T()); else if(match('-')) ... else break; } return L; }
Node* T() { Node* L=F(); while(1){ if(match('*')) L=new Node("OP","*",L,F()); else if(match('/')) ... else break; } return L; }
Node* F() { skipSpaces(); if(match('(')) { Node* n=E(); match(')'); return n; } if(substr(pos,2)=="id"){pos+=2;return new Node("ID","id");} if(substr(pos,3)=="num"){pos+=3;return new Node("NUM","num");} error; }

int eval(Node* r) { if(r->type=="ID") return 10; if(r->type=="NUM") return 5; int l=eval(r->L), R=eval(r->R); if(r->value=="+") return l+R; ... }
```

## Template 2: Flex Lexer (que2.l style)

```lex
%{ #include <stdio.h> %}
DIGIT [0-9]
ID [a-zA-Z_][a-zA-Z0-9_]*
%%
"int"|"float"|"if"|"else"  { printf("KEYWORD: %s\n", yytext); }
{ID}      { printf("IDENTIFIER: %s\n", yytext); }
{DIGIT}+  { printf("NUMBER: %s\n", yytext); }
"=="|"!=" { printf("RELOP: %s\n", yytext); }
"="       { printf("ASSIGN: %s\n", yytext); }
"+"|"-"|"*"|"/" { printf("ARITHOP: %s\n", yytext); }
";"|","   { printf("PUNCT: %s\n", yytext); }
[ \t\n]+  { }
.         { printf("UNKNOWN: %s\n", yytext); }
%%
int yywrap() { return 1; }
int main() { yylex(); return 0; }
```

---

# KEY CONCEPTS CHECKLIST

- [ ] Left recursion elimination: `E -> E+T` → `E -> T E'`
- [ ] Precedence: `E` calls `T` calls `F` (lower = higher precedence)
- [ ] Associativity: left-associative via `left = new Node(op, left, right)`
- [ ] `match()` pattern: skipSpaces → check → advance → return bool
- [ ] `F()` is base case: handles `(` `)` recursion, `id`, `num`
- [ ] AST evaluation = post-order traversal (children first)
- [ ] Flex: rule order matters for same-length matches
- [ ] Flex: maximal munch is automatic
- [ ] Flex: keywords BEFORE identifiers
- [ ] OOP AST: virtual methods, RAII destructors, exception handling

---

# COMMON EXAM VARIATIONS

| Variation | What Changes |
|-----------|--------------|
| Add unary minus | `F -> - F | (E) | id | num` |
| Add modulo `%` | Add to `T'` rule |
| Add power `^` | New level between `F` and `T` (right-assoc!) |
| Variables | Symbol table instead of hardcoded map |
| Error recovery | Don't exit, sync at `;` or `}` |
| Float numbers | Lexer: `{DIGIT}+"."{DIGIT}+` |
| String literals | Lexer: `\"([^\"\\]|\\.)*\"` |

---

# QUICK REFERENCE

```
GRAMMAR:
E  -> T E'
E' -> + T E' | - T E' | ε
T  -> F T'
T' -> * F T' | / F T' | ε
F  -> ( E ) | id | num

PARSE ORDER: E → T → F → (E) | id | num
PRECEDENCE:  * /  >  + -  (lower in tree = first evaluated)
ASSOCIATIVITY: All LEFT (prev result becomes left child)

FLEX RULES ORDER:
1. Keywords
2. Multi-char operators (==, !=, <=, >=)
3. Single-char operators (=, +, -, *, /)
4. Identifiers, Numbers
5. Punctuators
6. Whitespace (ignore)
7. Catch-all (.) LAST

COMPILE FLEX:
flex file.l
gcc lex.yy.c -o lexer -lfl
```