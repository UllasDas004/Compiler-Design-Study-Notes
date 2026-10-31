# LAB 7 — Semantic Analysis & Type Checking

## Question

Write a program to implement a semantic analyzer that performs **type checking** and **semantic error detection**
on an Abstract Syntax Tree (AST) produced by a parser.

Files: `que1.cpp` (hardcoded AST) and `que_1.cpp` (interactive input parser)

---

## What is Semantic Analysis?

After parsing (syntax checking), the compiler checks for **meaning** errors:
- Is this variable declared before use?
- Do the types match in this expression?
- Is this variable declared twice?

These errors can't be caught by grammar rules alone — they need context (a **symbol table**).

---

## Key Components

### 1. The AST Node Types
```cpp
enum NodeType {
    PROGRAM,  // root node that chains statements together
    DECL,     // variable declaration (int x;)
    ASSIGN,   // assignment (x = ...)
    BINOP,    // binary operation (x + y, a * b)
    ID,       // identifier (variable reference)
    CONST     // constant (literal number like 10, 3.14)
};

enum DataType {
    INT_T,    // int type
    FLOAT_T,  // float type
    VOID_T,   // no type (for statements)
    ERR_T     // error type (type mismatch)
};
```

### 2. The Symbol Table
```cpp
map<string, DataType> symbolTable;
// Maps variable names to their declared types
// e.g., {"x": INT_T, "y": FLOAT_T}
```

### 3. The `analyze()` Function — Core of Semantic Analysis

This walks the AST recursively and returns the **type** of each node:

```cpp
DataType analyze(ASTNode* node) {
    if (!node) return VOID_T;

    // PROGRAM node: analyze both subtrees (chained statements)
    if (node->nodeType == PROGRAM) {
        analyze(node->left);   // analyze first statement
        analyze(node->right);  // analyze rest of program
        return VOID_T;
    }

    // DECL: check for redeclaration, then add to symbol table
    if (node->nodeType == DECL) {
        if (symbolTable.find(node->value) != symbolTable.end())
            ERROR: Redeclaration!
        else
            symbolTable[node->value] = node->dataType;  // register variable
        return VOID_T;
    }

    // ID: check that variable was declared
    if (node->nodeType == ID) {
        if (symbolTable.find(node->value) == symbolTable.end())
            ERROR: Undeclared variable!
        return symbolTable[node->value];  // return its declared type
    }

    // CONST: its type is already stored in the node
    if (node->nodeType == CONST)
        return node->dataType;

    // BINOP: check both operands have the SAME type
    if (node->nodeType == BINOP) {
        DataType leftType  = analyze(node->left);
        DataType rightType = analyze(node->right);
        if (leftType != rightType)
            ERROR: Type mismatch in operation!
        return leftType;  // result type = same as operands
    }

    // ASSIGN: check LHS and RHS types match
    if (node->nodeType == ASSIGN) {
        DataType leftType  = analyze(node->left);   // variable type
        DataType rightType = analyze(node->right);  // expression type
        if (leftType != rightType)
            ERROR: Type mismatch in assignment!
        SUCCESS
        return leftType;
    }
}
```

---

## `que1.cpp` — Hardcoded AST Example

The program manually builds this AST to simulate:

```cpp
// Program being simulated:
// int x;
// x = 10;
// float y;
// y = x + 5.5;   // ERROR: int + float type mismatch
// z = 20;         // ERROR: z undeclared
// int x;          // ERROR: x redeclared
```

### Building the AST manually:
```cpp
ASTNode* n1 = makeNode(DECL, INT_T, "x", null, null);       // int x;
ASTNode* n2 = makeNode(ASSIGN, VOID_T, "=",
                 makeNode(ID, VOID_T, "x", null, null),      // x
                 makeNode(CONST, INT_T, "10", null, null));  // = 10;

ASTNode* n3 = makeNode(DECL, FLOAT_T, "y", null, null);     // float y;
ASTNode* n4 = makeNode(ASSIGN, VOID_T, "=",
                 makeNode(ID, VOID_T, "y", null, null),      // y
                 makeNode(BINOP, VOID_T, "+",
                     makeNode(ID, VOID_T, "x", null, null),   // x +
                     makeNode(CONST, FLOAT_T, "5.5", null, null))); // 5.5

// Chain statements into a program using PROGRAM nodes:
ASTNode* root = makeNode(PROGRAM, VOID_T, "", n1,
                makeNode(PROGRAM, VOID_T, "", n2,
                ...));

analyze(root);  // walk and check the tree
```

### Expected Output
```
Success: Declared 'x'
Success: Assignment to 'x' is semantically valid.
Success: Declared 'y'
Semantic Error: Type mismatch in '+' operation        <- x(int) + 5.5(float) mismatch
Semantic Error: Undeclared variable 'z'               <- z never declared
Semantic Error: Redeclaration of variable 'x'         <- int x declared twice
```

---

## `que_1.cpp` — Interactive Version

Accepts input line by line from the user.
Supported formats:
- `int varname` → DECL node
- `float varname` → DECL node  
- `var = operand` → ASSIGN (simple)
- `var = op1 OPERATOR op2` → ASSIGN with BINOP

```
> int x
> float y
> x = 10
> y = x + 5.5
> exit
--- Starting Semantic Analysis ---
```

### Parser logic in `que_1.cpp`
```cpp
vector<string> tokens = split(line);   // tokenize by spaces

if (tokens[0] == "int")   return DECL(INT_T, tokens[1]);
if (tokens[0] == "float") return DECL(FLOAT_T, tokens[1]);

if (tokens[1] == "=") {
    if (tokens.size() == 3)             // var = operand
        return ASSIGN(ID(tokens[0]), parseOperand(tokens[2]));
    if (tokens.size() == 5)             // var = op1 BINOP op2
        return ASSIGN(ID(tokens[0]),
                      BINOP(tokens[3], parseOperand(tokens[2]), parseOperand(tokens[4])));
}
```

`parseOperand()` checks if the token is a number (CONST) or identifier (ID):
```cpp
ASTNode* parseOperand(string s) {
    if (isNumber(s))     // s contains only digits or '.'
        return CONST(getConstType(s), s);   // INT_T if no '.', FLOAT_T if has '.'
    return ID(s);
}
```

---

## Visual: AST for `y = x + 5.5`

```
        ASSIGN
       /       \
    ID(y)      BINOP(+)
               /      \
            ID(x)    CONST(5.5)
            [INT]     [FLOAT]
              ^-----mismatch!-----^
```

When `analyze(BINOP(+))` runs:
- `analyze(ID(x))` → returns `INT_T` (from symbol table)
- `analyze(CONST(5.5))` → returns `FLOAT_T`
- `INT_T != FLOAT_T` → TYPE MISMATCH ERROR

---

## The `makeNode()` Helper

```cpp
ASTNode* makeNode(NodeType nt, DataType dt, string val, ASTNode* l, ASTNode* r) {
    ASTNode* node = new ASTNode();
    node->nodeType = nt;
    node->dataType = dt;
    node->value    = val;
    node->left     = l;
    node->right    = r;
    return node;
}
```

---

## Semantic Checks Summary

| Error Type | When it happens | Example |
|-----------|----------------|---------|
| Undeclared variable | ID node not in symbolTable | Using `z` before `int z;` |
| Redeclaration | DECL node already in symbolTable | `int x;` twice |
| Type mismatch (BINOP) | left type != right type | `x(int) + y(float)` |
| Type mismatch (ASSIGN) | var type != expr type | `int x; x = 3.14;` |

---

## Exam Tips

- The **symbol table** (`map<string, DataType>`) is the heart of semantic analysis
- `analyze()` returns `DataType` and is called recursively on the tree
- `ERR_T` propagates: if any child returns ERR_T, the parent also returns ERR_T (avoids cascade errors)
- `PROGRAM` nodes are just "links" — they chain multiple statements together
- Key insight: **declaration populates** the symbol table, **ID lookup uses** it
