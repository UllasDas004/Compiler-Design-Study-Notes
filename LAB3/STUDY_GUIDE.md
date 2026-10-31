# LAB 3 — Recursive Descent Parser & AST

## Questions

| File | What It Does |
|------|-------------|
| `que1.cpp` | Recursive descent parser, builds AST (tree nodes with struct), evaluates expressions |
| `temp.cpp` | Same but with OOP-style AST (classes with virtual functions), cleaner design |
| `que2.l` | Flex lexer for token recognition |

---

## The Grammar

```
E  -> T E'
E' -> + T E' | - T E' | epsilon
T  -> F T'
T' -> * F T' | / F T' | epsilon
F  -> ( E ) | id | num
```

This is a standard **left-recursion-free** grammar for arithmetic expressions.
- `E` (Expression) handles `+` and `-`
- `T` (Term) handles `*` and `/`
- `F` (Factor) handles atoms: `id`, `num`, or parenthesized expressions

### Why This Grammar?
Direct left-recursion like `E -> E + T` breaks recursive descent parsers.
We eliminate it by using the `E'` (E-prime) trick:
```
E  -> E + T     becomes:    E  -> T E'
                             E' -> + T E' | epsilon
```

---

## How Recursive Descent Works

Each grammar rule becomes ONE function:

```
E  -> T E'       =>   Node* E()  { ... calls T() and handles + or - }
T  -> F T'       =>   Node* T()  { ... calls F() and handles * or / }
F  -> (E)|id|num =>   Node* F()  { ... handles base cases }
```

The parser reads tokens left-to-right. Each function:
1. Calls the appropriate sub-functions
2. Checks the current character for its operator
3. Returns a tree node

---

## que1.cpp — Simple Struct-Based Parser

### Data Structure
```cpp
struct Node {
    string type;   // "OP", "ID", "NUM"
    string value;  // "+", "-", "id", "num"
    Node* left;
    Node* right;
};
```

### E() Function (handles + and -)
```cpp
Node* E() {
    Node* left = T();          // Parse the first term

    while (true) {
        if (match('+')) {      // If we see '+'
            Node* right = T(); // Parse right term
            left = new Node("OP", "+", left, right);  // Build subtree
        }
        else if (match('-')) { ... }
        else break;
    }

    return left;
}
```

### T() Function (handles * and /)
```cpp
Node* T() {
    Node* left = F();          // Parse the first factor

    while (true) {
        if (match('*')) {
            Node* right = F();
            left = new Node("OP", "*", left, right);
        }
        else if (match('/')) { ... }
        else break;
    }

    return left;
}
```

### F() Function (base case)
```cpp
Node* F() {
    if (match('(')) {          // Parenthesized: (E)
        Node* node = E();
        if (!match(')')) error;
        return node;
    }
    if (substr(pos,2) == "id") {  pos+=2; return new Node("ID","id"); }
    if (substr(pos,3) == "num") { pos+=3; return new Node("NUM","num"); }
}
```

### Evaluate Function (tree walk)
```cpp
int evaluate(Node* root) {
    if (root->type == "ID")  return values["id"];   // id=10 (hardcoded)
    if (root->type == "NUM") return values["num"];  // num=5 (hardcoded)

    int left  = evaluate(root->left);
    int right = evaluate(root->right);

    if (root->value == "+") return left + right;
    if (root->value == "-") return left - right;
    if (root->value == "*") return left * right;
    if (root->value == "/") return left / right;
}
```

### Sample Output for `id + id * num` (id=10, num=5)
```
Expression: id + id * num
AST:
+
  id
  *
    id
    num
Value: 60       (10 + 10*5 = 60, * binds tighter than +)
```

```
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

## temp.cpp — OOP-Style Parser (Cleaner Version)

### Uses polymorphic classes instead of a single struct:
```cpp
class ASTNode { virtual int evaluate(...) = 0; virtual void print() = 0; };
class IdNode  : public ASTNode { ... };
class NumNode : public ASTNode { ... };
class BinaryOpNode : public ASTNode { char op; ASTNode* left; ASTNode* right; ... };
```

### Same grammar, but uses a token-based lexer (not string position):
- Lexer class produces tokens: `ID`, `NUM`, `PLUS`, `MINUS`, `MUL`, `DIV`
- Parser class consumes tokens using `advance()` and `match()`

---

## Visual: AST for `id + id * num`

```
        +
       / \
     id   *
         / \
        id  num
```

The tree structure **encodes operator precedence**:
- `*` is **lower** in the tree (evaluated first, during bottom-up traversal)
- `+` is **higher** (evaluated after children are done)

---

## Exam Tips

- Each grammar rule = one function in recursive descent
- The key to precedence: `E` calls `T`, `T` calls `F` — lower-precedence operators are higher in the call stack
- `match(char)` = checks current character and advances if it matches
- `skipSpaces()` = must be called before checking next token
- `pos` = global position in the input string
