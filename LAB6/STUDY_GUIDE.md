# LAB 6 — Flex & Bison (Lex/Yacc)

## Questions

| Files | Question |
|-------|----------|
| `config.l` + `config.y` | Syntax checker and evaluator for a config/scripting language with variable assignments |
| `math.l` + `math.y` | Syntax checker for mathematical expressions with balanced symbols `()`, `[]`, `{}` |

---

## Big Picture: What is Flex/Bison?

Think of it as splitting your compiler's frontend into two separate tools:

```
Source Code
     |
     v
[FLEX/LEX]  -- Lexical Analyzer -- Splits text into tokens
     |
     v (token stream)
     |
[BISON/YACC] -- Parser -- Checks grammar and performs actions
     |
     v
  Output / Evaluation
```

- **Flex** (`.l` files) = writes the lexer for you. You give it patterns (regex), it generates C code.
- **Bison** (`.y` files) = writes the parser for you. You give it grammar rules, it generates a C parser.

---

## Part 1 — Understanding `.l` (Flex) Files

A flex file has 3 sections separated by `%%`:

```
%{
  /* C code to include at top */
%}

%%
/* Rules: regex  {  action code  } */
%%

/* C functions */
```

### `config.l` Breakdown

```lex
%{
#include "config.tab.h"    // Include bison's generated header (has token IDs)
#include <string.h>
#include <stdlib.h>
%}

%%

[a-zA-Z_][a-zA-Z0-9_]* {   // Regex: identifier (starts with letter/underscore)
    yylval.str = strdup(yytext);  // Copy the matched text into yylval.str
    return IDENTIFIER;            // Tell bison "I found an IDENTIFIER token"
}

[0-9]+ {                    // Regex: integer number
    yylval.num = atoi(yytext);    // Convert to int, store in yylval.num
    return NUMBER;                // Tell bison "I found a NUMBER token"
}

[-+*/=;\n] {                // Single-char operators and newline
    return yytext[0];             // Return the character itself as the token
}

[ \t]+ {                    // Whitespace: spaces and tabs -> SKIP (no return)
    // Do nothing, just consume
}

. {                         // Any other character -> SKIP
    // Do nothing
}

%%

int yywrap() { return 1; }  // Called when input ends; returning 1 means stop
```

### Key Variables in Flex

| Variable | What it is |
|----------|-----------|
| `yytext` | The matched text (a char*) |
| `yyleng` | Length of the match |
| `yylval` | Union that passes values to bison (shared variable) |
| `yylex()` | The generated function that bison calls to get next token |

---

## Part 2 — Understanding `.y` (Bison) Files

A bison file also has 3 sections:

```
%{
  /* C code to include at top */
%}

/* declarations: token types, precedence, etc. */

%%
/* Grammar rules */
%%

/* Additional C code */
```

### `config.y` Breakdown

```yacc
%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int yylex();
void yyerror(const char *s);

// ===== SYMBOL TABLE =====
char vars[100][50];   // stores variable names
int  vals[100];       // stores variable values
int  var_count = 0;

// Look up a variable's value
int get_var(char *name) {
    for(int i = 0; i < var_count; i++)
        if(strcmp(vars[i], name) == 0)
            return vals[i];
    return 0;  // default value if not found
}

// Set a variable's value (update if exists, add if new)
void set_var(char *name, int val) {
    for(int i = 0; i < var_count; i++)
        if(strcmp(vars[i], name) == 0) { vals[i] = val; return; }
    strcpy(vars[var_count], name);
    vals[var_count++] = val;
}
%}

// ===== THE UNION =====
// yylval can hold EITHER a string OR an int (union = shared memory)
%union {
    int num;    // for NUMBER tokens
    char* str;  // for IDENTIFIER tokens
}

// ===== TOKEN DECLARATIONS =====
%token <str> IDENTIFIER   // IDENTIFIER token carries a string value
%token <num> NUMBER       // NUMBER token carries an int value
%type  <num> expr         // expr non-terminal produces an int value

// ===== OPERATOR PRECEDENCE (low to high) =====
%left '+' '-'     // + and - have equal precedence, left-associative
%left '*' '/'     // * and / have higher precedence, left-associative

%%

// ===== GRAMMAR RULES =====

program:
    program statement '\n'    // multiple statements (recursive)
    | program '\n'            // blank lines are OK
    | statement '\n'          // single statement
    | '\n'                    // blank line
    ;

statement:
    IDENTIFIER '=' expr ';' {
        // Action: assign the value of expr to the variable
        set_var($1, $3);                              // $1=IDENTIFIER, $3=expr value
        printf("Success: Assigned %s = %d\n", $1, $3);
        free($1);  // free the strdup'd string from flex
    }
    | expr ';' {
        // Action: just evaluate and print the result
        printf("Evaluated Result: %d\n", $1);         // $1=expr value
    }
    ;

expr:
    NUMBER              { $$ = $1; }             // expr -> NUMBER:     value is the number
    | IDENTIFIER        { $$ = get_var($1); free($1); }  // look up variable
    | expr '+' expr     { $$ = $1 + $3; }        // addition
    | expr '-' expr     { $$ = $1 - $3; }        // subtraction
    | expr '*' expr     { $$ = $1 * $3; }        // multiplication
    | expr '/' expr     { $$ = $1 / $3; }        // division
    | '(' expr ')'      { $$ = $2; }             // parentheses: value is inner expr
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Syntax Error: %s\n", s);
}

int main() {
    yyparse();    // Start parsing (calls yylex() internally for each token)
    return 0;
}
```

---

## Part 3 — `math.l` + `math.y` (Expression Validator)

This simpler pair validates whether a math expression has correct structure and balanced brackets.

### `math.l` — Tokens it recognizes
```lex
[0-9]+(\.[0-9]+)?  { return NUMBER; }   // integers and decimals
[a-zA-Z_][a-zA-Z0-9_]* { return ID; }  // identifiers
[-+*/()[\]{}\n]    { return yytext[0]; } // operators and all bracket types
[ \t]+             { }                   // skip whitespace
.                  { return INVALID; }   // anything else = invalid
```

### `math.y` — Grammar rules
```yacc
%token NUMBER ID INVALID
%left '+' '-'
%left '*' '/'

%%
input:
    input line | line
    ;

line:
    expr '\n' {
        if (valid) printf("Valid: Correct mathematical structure.\n");
        valid = 1;
    }
    | error '\n' {
        valid = 1;
        yyerrok;   // reset error state so parsing continues
    }
    | '\n'     // blank lines OK
    ;

expr:
    NUMBER | ID
    | expr '+' expr   | expr '-' expr
    | expr '*' expr   | expr '/' expr
    | '(' expr ')'    // round brackets
    | '[' expr ']'    // square brackets
    | '{' expr '}'    // curly brackets
    ;
%%
```

Key: `error '\n'` is a Bison built-in error recovery rule. When a syntax error is found,
bison looks for this `error` token to recover and continue parsing the next line.

---

## How to Compile and Run

### For `config.l` + `config.y`:
```bash
flex config.l              # generates lex.yy.c
bison -d config.y          # generates config.tab.c and config.tab.h (-d = create header)
gcc lex.yy.c config.tab.c -o config_eval -lfl
./config_eval
```

### For `math.l` + `math.y`:
```bash
flex math.l
bison -d math.y
gcc lex.yy.c math.tab.c -o math_validator -lfl
./math_validator
```

---

## Sample Sessions

### config_eval input:
```
x = 5 + 3;
y = x * 2;
y + 1;
```
Output:
```
Success: Assigned x = 8
Success: Assigned y = 16
Evaluated Result: 17
```

### math_validator input:
```
3 + 4 * 2
(a + b) * [c - d]
{x + (y * z}
```
Output:
```
Valid: Correct mathematical structure and balanced symbols.
Valid: Correct mathematical structure and balanced symbols.
Syntax Error: Invalid expression or unbalanced parentheses/brackets.
```

---

## The Dollar Sign Notation in Bison Rules

This is the most confusing part — let's demystify it:

```
expr '+' expr     { $$ = $1 + $3; }
```

- `$$` = the **value to assign to the left-hand side** (the result of this production, here: `expr`)
- `$1` = value of the **1st symbol** on the right side (first `expr`)
- `$2` = value of the **2nd symbol** (`'+'`)
- `$3` = value of the **3rd symbol** (second `expr`)

```
IDENTIFIER '=' expr ';'  { set_var($1, $3); }
     ^1          ^3
```

- `$1` = the IDENTIFIER's string (from `yylval.str`)
- `$2` = the `=` character (rarely used)
- `$3` = the expr's computed integer value

---

## Key Concepts Cheatsheet

| Concept | Flex Side | Bison Side |
|---------|-----------|-----------|
| Communication channel | Writes to `yylval` | Reads from `yylval` |
| Token IDs | `return IDENTIFIER;` | `%token IDENTIFIER` |
| Token values | `yylval.str = strdup(yytext)` | `$1`, `$2`, etc. |
| Grammar actions | N/A | `{ $$ = $1 + $3; }` |
| Error handling | N/A | `yyerror()`, `error` token |
| Input source | stdin by default | calls `yylex()` |

---

## Exam Tips

- **Flex rule order matters**: more specific rules first (put keywords before identifiers)
- **`%left`/`%right`** in bison = operator precedence and associativity
- **`%union`** = because tokens can carry different types (int OR string)
- **`%token <str>`** = this token carries the `str` part of the union
- **`%type <num>`** = this non-terminal produces a `num` value
- **`yywrap()`** returning 1 = tell flex "no more input, stop"
- **`yyerrok`** = reset error status so bison can continue parsing after an error
- The **`-d` flag** in `bison -d` generates the `.tab.h` header that flex needs for token IDs
