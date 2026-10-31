# LAB 4 — LL(1) Parsing & Left Recursion Elimination

## Questions

| File | Question |
|------|----------|
| `que_1.cpp` | Build LL(1) parsing table + table-driven predictive parser |
| `que1.cpp` | Same as above with more comments (study from this one) |
| `que2.cpp` | Eliminate left recursion from a grammar |

---

## Grammar Used (que_1.cpp / que1.cpp)

```
S -> a A B e
A -> b A c | d
B -> f | epsilon
```

---

## Part 1 — LL(1) Parsing Table (`que_1.cpp`)

### Big Picture: What is LL(1) Parsing?

- **L**eft-to-right scan, **L**eftmost derivation, **1** token of lookahead
- Instead of trying all productions (backtracking), we use a **table** to directly pick which production to apply
- The table is built from **FIRST** and **FOLLOW** sets

### Step 1: FIRST Sets

**FIRST(X)** = set of terminals that can appear as the **first symbol** of any string derived from X

Rules:
1. If X is a terminal: `FIRST(X) = {X}`
2. If X -> eps: add `eps` to FIRST(X)  
3. If X -> Y Z ...: add FIRST(Y) (minus eps) to FIRST(X); if Y can be eps, also add FIRST(Z), etc.

For our grammar:
```
FIRST(S) = { a }               (S -> a A B e, starts with terminal 'a')
FIRST(A) = { b, d }            (A -> bAc starts with b; A -> d starts with d)
FIRST(B) = { f, eps }          (B -> f; B -> eps)
```

### Step 2: FOLLOW Sets

**FOLLOW(X)** = set of terminals that can appear **immediately after** X in some sentential form

Rules:
1. `FOLLOW(start symbol)` gets `$` (end of input)
2. If `A -> alpha B beta`: add FIRST(beta) - {eps} to FOLLOW(B)
3. If `A -> alpha B` (B is last) OR FIRST(beta) contains eps: add FOLLOW(A) to FOLLOW(B)

For our grammar:
```
FOLLOW(S) = { $ }
FOLLOW(A) = { f, e, $ }        (In S -> aABe: after A comes B; FIRST(B) = {f,eps}; if eps, then follow 'e')
FOLLOW(B) = { e }              (In S -> aABe: after B comes 'e')
```

### Step 3: Build the Parsing Table

For each production `A -> alpha`:
- For each terminal `a` in FIRST(alpha): add `A -> alpha` to `M[A, a]`
- If eps in FIRST(alpha): for each `b` in FOLLOW(A): add `A -> alpha` to `M[A, b]`

```
       |  a  |  b  |  c  |  d  |  e  |  f  |  $
-------+-----+-----+-----+-----+-----+-----+-----
  S    | aABe|     |     |     |     |     |
  A    |     | bAc |     |  d  |     |     |
  B    |     |     |     |     | eps |  f  |
```

> [!NOTE]
> `B -> eps` goes into `M[B, e]` because eps is in FIRST(B) and `e` is in FOLLOW(B)

### Step 4: Table-Driven Parsing

**Data structures:**
- `stack`: starts with `[$, S]` ($ at bottom, start symbol on top)
- `input`: tokens + `$` at end

**Algorithm:**
```
while stack not empty:
    top = stack.top()
    current = input[pointer]

    if top == current == '$':  ACCEPT
    if top == current:         Match -> pop stack, advance input
    if M[top][current] exists: Expand -> pop top, push production in reverse
    else:                      ERROR
```

### Parsing Trace for input `adfe`

```
Stack               Input               Action
------------------------------------------------------------
$S                  adfe$               S -> aABe
$eBAa               adfe$               Match a
$eBAa               dfe$                ... (continues)
```

### Code Structure (que1.cpp)
```cpp
// Types
using Grammar = map<string, vector<vector<string>>>;
using Sets    = map<string, set<string>>;
using ParsingTable = map<string, map<string, vector<string>>>;

Sets first  = calculateFirst(grammar);
Sets follow = calculateFollow(grammar, first, startSymbol);
ParsingTable table = buildParsingTable(grammar, first, follow);
parse(input, table, startSymbol);
```

---

## Part 2 — Eliminating Left Recursion (`que2.cpp`)

### Problem: LL(1) parsers can't handle left-recursive grammars!

A grammar is **left-recursive** if `A -> A alpha` (A appears at the start of its own production).
This causes infinite recursion in a recursive descent parser.

### Original Grammar (left-recursive)
```
E -> E + T | T
T -> T * F | F
F -> ( E ) | id
```

### How to Eliminate Left Recursion

For a rule `A -> A alpha | beta` (alpha = what follows A, beta = non-recursive alternative):

**Transform to:**
```
A  -> beta A'
A' -> alpha A' | eps
```

### Transformed Grammar
```
E  -> T E'
E' -> + T E' | eps

T  -> F T'
T' -> * F T' | eps

F  -> ( E ) | id
```

### Verification
Original: `E => E + T => T + T`  (starts with E again -> infinite!)
Fixed:    `E => T E' => F T' E' => id T' E'`  (no left recursion!)

### Code Pattern
```cpp
// Separate productions into recursive and non-recursive
for (const auto& production : productions) {
    if (!production.empty() && production[0] == nonTerminal) {
        // This is left-recursive: A -> A alpha
        // alpha = production[1:]
        recursiveProductions.push_back(alpha);
    } else {
        // This is non-recursive: A -> beta
        nonRecursiveProductions.push_back(production);
    }
}

// Create new rules:
// A  -> beta A'   (for each beta)
// A' -> alpha A'  (for each alpha)
// A' -> eps
```

---

## Complete FIRST/FOLLOW Summary for Our Grammar

```
Grammar: S -> aABe, A -> bAc | d, B -> f | eps

FIRST(S) = { a }
FIRST(A) = { b, d }
FIRST(B) = { f, eps }

FOLLOW(S) = { $ }
FOLLOW(A) = { f, e }
FOLLOW(B) = { e }
```

---

## Exam Tips

- FIRST set: "what terminals can this non-terminal START with?"
- FOLLOW set: "what terminals can come AFTER this non-terminal?"
- If FIRST contains eps, we also look at FOLLOW for the table entry
- Left recursion = non-terminal appears as the first symbol on the right of its own rule
- The fix: separate into recursive (`A -> A alpha`) and base (`A -> beta`) productions
