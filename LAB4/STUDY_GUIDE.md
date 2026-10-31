# LAB 4 — Complete Study Guide
## LL(1) Parsing Table Construction & Left Recursion Elimination

---

## Files At a Glance

| File | Purpose | Focus |
|------|---------|-------|
| `que_1.cpp` | **LL(1) Parser** | FIRST/FOLLOW sets, parsing table, table-driven predictive parser |
| `que2.cpp` | **Left Recursion Elimination** | Transform left-recursive grammar to LL(1)-compatible form |

---

# PART 1 — LL(1) PARSING TABLE (`que_1.cpp`)

## The Grammar

```
S -> a A B e
A -> b A c | d
B -> f | ε
```

**Terminals:** `a, b, c, d, e, f`
**Non-terminals:** `S, A, B`
**Start Symbol:** `S`

---

## LL(1) Parsing Overview

```
┌─────────────────────────────────────────────────────────┐
│  LL(1) = Left-to-right, Leftmost derivation, 1 lookahead │
└─────────────────────────────────────────────────────────┘

Instead of backtracking, we build a TABLE that tells us exactly
which production to use based on (current non-terminal, lookahead token).
```

### Pipeline

```
Grammar → FIRST sets → FOLLOW sets → Parsing Table → Table-Driven Parser
```

---

## Step 1: FIRST Sets

### Definition
**FIRST(X)** = set of terminals that can appear as the **first symbol** of any string derived from X

### Rules
1. **Terminal:** `FIRST(a) = {a}`
2. **Epsilon:** `FIRST(ε) = {ε}`
3. **Non-terminal X → Y₁ Y₂ ... Yₖ:**
   - Add `FIRST(Y₁) - {ε}` to `FIRST(X)`
   - If `ε ∈ FIRST(Y₁)`, also add `FIRST(Y₂) - {ε}`, etc.
   - If **all** Yᵢ can derive ε, add `ε` to `FIRST(X)`

### Computation for Our Grammar

| Non-terminal | Productions | FIRST |
|--------------|-------------|-------|
| `B` | `B → f` | `{f}` |
| | `B → ε` | `{ε}` |
| **B** | | **{f, ε}** |
| `A` | `A → b A c` | `{b}` |
| | `A → d` | `{d}` |
| **A** | | **{b, d}** |
| `S` | `S → a A B e` | `{a}` |
| **S** | | **{a}** |

---

## Step 2: FOLLOW Sets

### Definition
**FOLLOW(A)** = set of terminals that can appear **immediately after A** in some sentential form

### Rules
1. **Start symbol:** `FOLLOW(S) = {$}` (where `$` = end of input)
2. **If `A → α B β`:** Add `FIRST(β) - {ε}` to `FOLLOW(B)`
3. **If `A → α B`** (B is last) **OR** `ε ∈ FIRST(β)`: Add `FOLLOW(A)` to `FOLLOW(B)`

### Computation for Our Grammar

| Non-terminal | Derivation | FOLLOW |
|--------------|------------|--------|
| `S` | Start symbol | `{$}` |
| `A` | `S → a A B e` → after A comes B | `FIRST(B)-{ε} = {f}` |
| | `S → a A B e` and `ε ∈ FIRST(B)` | `FOLLOW(S) = {$}` |
| | `A → b A c` → after A comes `c` | `{c}` |
| **A** | | **{c, e, f, $}** |
| `B` | `S → a A B e` → after B comes `e` | `{e}` |
| **B** | | **{e}** |

> **Note:** The code output shows `FOLLOW(A) = {c e f}` (missing `$`). This is because the grammar doesn't have a production where A is followed by something that derives ε and then end of input. The `$` comes from `FOLLOW(S)` but only propagates if A is at the end of a production in S. In `S → a A B e`, A is followed by B, not end of input. Since B can derive ε, A is followed by e, not $.

---

## Step 3: LL(1) Parsing Table

### Algorithm
For each production `A → α`:
1. For each `a ∈ FIRST(α) - {ε}`: `M[A, a] = α`
2. If `ε ∈ FIRST(α)`: For each `b ∈ FOLLOW(A)`: `M[A, b] = α`

### Our Parsing Table

| | a | b | c | d | e | f | $ |
|---|---|---|---|---|---|---|---|
| **S** | `a A B e` | | | | | | |
| **A** | | `b A c` | | `d` | | | |
| **B** | | | | | `ε` | `f` | |

> **Key:** `B → ε` goes into `M[B, e]` because `ε ∈ FIRST(B)` and `e ∈ FOLLOW(B)`

### Conflict Check
If any cell `M[A, a]` gets **multiple productions** → Grammar is **NOT LL(1)**

---

## Step 4: Table-Driven Predictive Parsing

### Data Structures
```
stack:  [$, S]          (bottom to top, top at back of vector)
input:  [a, d, f, e, $] (tokens + end marker)
```

### Algorithm
```
while stack not empty:
    top = stack.back()
    current = input[pointer]

    if top == "$" and current == "$":
        ACCEPT
    else if top == current:          # Terminal match
        MATCH: pop stack, pointer++
    else if M[top][current] exists:  # Non-terminal expansion
        EXPAND: pop top, push production in REVERSE
    else:
        ERROR
```

### Parsing Trace: Input `adfe`

```
Stack        Input        Action
--------------------------------------------------
$S           adfe$        S -> a A B e
$e B A a     adfe$        Match a
$e B A       dfe$         A -> d
$e B d       dfe$         Match d
$e B         fe$          B -> f
$e f         fe$          Match f
$e           e$           Match e
$            $            Accept
```

---

## Code Structure (`que_1.cpp`)

```cpp
// Type definitions
using Grammar      = map<string, vector<vector<string>>>;      // NT -> productions
using Sets         = map<string, set<string>>;                 // Symbol -> terminals
using ParsingTable = map<string, map<string, vector<string>>>; // M[NT][term] = prod

// Pipeline
Sets first  = calculateFirst(grammar);
Sets follow = calculateFollow(grammar, first, startSymbol);
ParsingTable table = buildParsingTable(grammar, first, follow);
parse(input, table, startSymbol);
```

---

## Exam Templates

### FIRST Set Calculation
```cpp
Sets first;
for (terminal t) first[t].insert(t);
first["eps"].insert("eps");

while (changed) {
    for (A -> X1 X2 ... Xk) {
        bool nullable = true;
        for (Xi in production) {
            first[A].insert(first[Xi] - {eps});
            if (!first[Xi].count("eps")) { nullable = false; break; }
        }
        if (nullable) first[A].insert("eps");
    }
}
```

### FOLLOW Set Calculation
```cpp
follow[start].insert("$");
while (changed) {
    for (A -> X1 ... Xk) {
        for (i = 0; i < k; i++) {
            if (Xi is non-terminal) {
                remaining = Xi+1 ... Xk;
                firstRem = firstOfSequence(remaining, first);
                follow[Xi].insert(firstRem - {eps});
                if (remaining.empty() || firstRem.count("eps")) {
                    follow[Xi].insert(follow[A]);
                }
            }
        }
    }
}
```

### Parsing Table
```cpp
for (A -> alpha) {
    for (a in firstOfSequence(alpha) - {eps})
        table[A][a] = alpha;
    if (firstOfSequence(alpha).count("eps"))
        for (b in follow[A])
            table[A][b] = alpha;
}
```

### Table-Driven Parser
```cpp
stack = {"$", start};
input = tokens + "$";
ptr = 0;

while (!stack.empty()) {
    top = stack.back(); cur = input[ptr];
    if (top == "$" && cur == "$") return ACCEPT;
    if (top == cur) { stack.pop_back(); ptr++; }
    else if (table[top].count(cur)) {
        prod = table[top][cur];
        stack.pop_back();
        if (!(prod.size()==1 && prod[0]=="eps"))
            for (i = prod.size()-1; i >= 0; i--) stack.push_back(prod[i]);
    }
    else return ERROR;
}
```

---

# PART 2 — LEFT RECURSION ELIMINATION (`que2.cpp`)

## The Problem

**Left Recursion:** `A → A α` (non-terminal appears as FIRST symbol of its own production)

```
Original: E -> E + T | T
          E() calls E() calls E()... INFINITE RECURSION!
```

LL(1) parsers (both recursive descent AND table-driven) **cannot handle left recursion**.

---

## Solution: Direct Left Recursion Elimination

### Pattern
```
Input:  A -> A α₁ | A α₂ | ... | β₁ | β₂ | ...
        (α = recursive parts, β = non-recursive parts)

Output: A  -> β₁ A' | β₂ A' | ...
        A' -> α₁ A' | α₂ A' | ... | ε
```

### Example

**Original (Left-Recursive):**
```
E -> E + T | T
T -> T * F | F
F -> ( E ) | id
```

**Step 1: Eliminate E recursion**
```
E  -> T E'
E' -> + T E' | ε
```

**Step 2: Eliminate T recursion**
```
T  -> F T'
T' -> * F T' | ε
```

**Result (LL(1)-Compatible):**
```
E  -> T E'
E' -> + T E' | ε

T  -> F T'
T' -> * F T' | ε

F  -> ( E ) | id
```

---

## Algorithm (Implemented in `que2.cpp`)

```cpp
Grammar eliminateLeftRecursion(const Grammar& grammar) {
    Grammar result;

    for (each nonTerminal A in grammar) {
        // 1. Partition productions
        recursive = [];      // A -> A alpha
        nonRecursive = [];   // A -> beta

        for (prod in A's productions) {
            if (prod[0] == A)  recursive.push_back(prod[1:]);   // alpha
            else               nonRecursive.push_back(prod);      // beta
        }

        // 2. If no recursion, keep as-is
        if (recursive.empty()) { result[A] = prods; continue; }

        // 3. Create A'
        A_prime = A + "'";

        // A -> beta A'  (for each beta)
        for (beta in nonRecursive)
            result[A].push_back(beta + A_prime);

        // A' -> alpha A'  (for each alpha)
        for (alpha in recursive)
            result[A_prime].push_back(alpha + A_prime);

        // A' -> ε
        result[A_prime].push_back({"eps"});
    }

    return result;
}
```

---

## Indirect Left Recursion (Bonus)

```
A -> B α
B -> A β
```

**Algorithm:**
1. Order non-terminals: A₁, A₂, ..., Aₙ
2. For i = 1 to n:
   - For j = 1 to i-1:
     - Replace Aᵢ -> Aⱼ γ with all expansions of Aⱼ
   - Eliminate direct left recursion from Aᵢ

---

## Complete FIRST/FOLLOW Summary

```
Grammar: S -> a A B e, A -> b A c | d, B -> f | ε

FIRST(S) = { a }
FIRST(A) = { b, d }
FIRST(B) = { f, ε }

FOLLOW(S) = { $ }
FOLLOW(A) = { c, e, f }
FOLLOW(B) = { e }
```

---

# KEY CONCEPTS CHECKLIST

- [ ] FIRST(terminal) = {terminal}, FIRST(ε) = {ε}
- [ ] FIRST(X) = FIRST(Y₁) ∪ (if ε∈FIRST(Y₁) then FIRST(Y₂)) ∪ ...
- [ ] FOLLOW(start) = {$}
- [ ] If A → αBβ: FOLLOW(B) += FIRST(β) - {ε}
- [ ] If A → αB or ε∈FIRST(β): FOLLOW(B) += FOLLOW(A)
- [ ] Parsing table: M[A,a] = α for a∈FIRST(α); if ε∈FIRST(α), also for b∈FOLLOW(A)
- [ ] LL(1) conflict = multiple entries in same M[A,a] cell
- [ ] Left recursion: A → Aα
- [ ] Direct elimination: A → Aα | β  ⇒  A → βA', A' → αA' | ε
- [ ] Table-driven parser: stack, match terminals, expand via table, push in REVERSE
- [ ] Accept when stack = [$] and input = [$]

---

# COMMON EXAM VARIATIONS

| Variation | What Changes |
|-----------|--------------|
| Multiple start symbols | Compute FOLLOW for each, use $ for all |
| ε-productions everywhere | More ε in FIRST, more FOLLOW entries in table |
| Ambiguous grammar | Table has conflicts - not LL(1) |
| Indirect left recursion | Need ordering + substitution before elimination |
| Left factoring needed | A → αβ₁ | αβ₂ ⇒ A → αA', A' → β₁ | β₂ |
| Semantic actions | Attach code to table entries or parser stack |
| Error recovery | Synchronize at FOLLOW sets (semicolons, braces) |

---

# QUICK REFERENCE

```
FIRST RULES:
  terminal a          → {a}
  ε                   → {ε}
  X → Y₁ Y₂ ... Yₖ    → FIRST(Y₁)-{ε} ∪ (if ε∈Y₁: FIRST(Y₂)-{ε}) ∪ ...

FOLLOW RULES:
  start symbol        → {$}
  A → α B β           → FOLLOW(B) += FIRST(β)-{ε}
  A → α B (B last) or ε∈FIRST(β) → FOLLOW(B) += FOLLOW(A)

PARSING TABLE:
  For A → α:
    for a in FIRST(α)-{ε}: M[A,a] = α
    if ε in FIRST(α):
      for b in FOLLOW(A): M[A,b] = α

PARSER:
  stack = [$, S]
  input = tokens + $
  loop:
    top, cur = stack.top(), input[ptr]
    if top==cur: pop, advance
    else if M[top][cur]: pop, push prod REVERSE
    else if top==cur=="$": ACCEPT
    else: ERROR

LEFT RECURSION:
  A → Aα | β  ⇒  A → βA', A' → αA' | ε

COMPILE:
  g++ que_1.cpp -o ll1_parser
  g++ que2.cpp -o left_rec_elim
```