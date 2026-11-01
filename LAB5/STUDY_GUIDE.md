# LAB 5 — Complete Study Guide
## LR(0) Parser Construction & Parsing

---

## Files At a Glance

| File | Purpose | Focus |
|------|---------|-------|
| `lr0_parser.cpp` | **Full LR(0) parser** | Item sets, closure, GOTO, ACTION/GOTO tables, parsing trace |
| `lr0_exam.cpp` | **Exam-friendly version** | Interactive input, compact code |

---

## The Grammar

```
S' -> S          (Rule 0: Augmented start)
S  -> A          (Rule 1)
A  -> A b        (Rule 2: Left-recursive)
A  -> a          (Rule 3)
```

**Terminals:** `a, b`
**Non-terminals:** `S, A, S'`
**Start Symbol:** `S` (augmented to `S'`)

**Input String:** `abb`

**Derivation:**
```
S => A => Ab => Abb => abb
    (Rule 1) (Rule 2) (Rule 3) (Rule 2)
```

---

## LR(0) Parsing Overview

```
┌─────────────────────────────────────────────────────────┐
│  LR(0) = Left-to-right, Rightmost derivation in reverse,│
│           0 tokens of lookahead                         │
└─────────────────────────────────────────────────────────┘

Bottom-up parsing:
  • SHIFT: push input token onto stack, move to new state
  • REDUCE: RHS complete → pop |RHS| symbols/states, push LHS via GOTO
  • ACCEPT: reduced S' -> S with $ on input

Uses FINITE AUTOMATON (states = LR(0) item sets) to decide actions.
```

---

## Step 1: LR(0) Items

An **LR(0) item** is a production with a **dot (·)** showing parsing progress:

```
Rule 0: S' -> .S      (haven't seen S)
Rule 0: S' -> S.      (seen S -> reduce to S')

Rule 1: S  -> .A      (haven't seen A)
Rule 1: S  -> A.      (seen A -> reduce to S)

Rule 2: A  -> .A b    (haven't seen anything for A->Ab)
Rule 2: A  -> A. b    (seen A, expecting b)
Rule 2: A  -> A b.    (seen Ab -> reduce to A)

Rule 3: A  -> .a      (haven't seen a)
Rule 3: A  -> a.      (seen a -> reduce to A)
```

---

## Step 2: Building States via Closure & GOTO

### Closure Operation

```
Closure(I) = For each item [A -> α · B β] in I,
             add all [B -> · γ] for every B-production.
             Repeat until no new items.
```

**Purpose:** When we're "in the middle of" A -> α B β, we must also
account for all ways B could start (i.e., all B-productions with dot at start).

### GOTO Operation

```
GOTO(I, X) = { [A -> α X · β] | [A -> α · X β] ∈ I }
             then take Closure of the result.
```

**Purpose:** The state we reach after "seeing" symbol X in state I.

### All States for Our Grammar

**I0 (Start State): `Closure({S' -> ·S})`**
```
S' -> ·S
S  -> ·A
A  -> ·A b
A  -> ·a
```

**GOTO(I0, S) = I1**
```
S' -> S·          ← ACCEPT STATE
```

**GOTO(I0, A) = I2**
```
S  -> A·
A  -> A·b
```

**GOTO(I0, a) = I3**
```
A  -> a·          ← REDUCE STATE (Rule 3)
```

**GOTO(I2, b) = I4**
```
A  -> A b·        ← REDUCE STATE (Rule 2)
```

---

## Step 3: LR(0) Parsing Table

| State | a | b | $ | S | A |
|-------|---|---|---|---|---|
| 0 | s3 | | | 2 | 1 |
| 1 | | s4 | | | |
| 2 | r1/r3 | s4 | r1/r3 | | |
| 3 | r3 | r3 | r3 | | |
| 4 | r2 | r2 | r2 | | |

**Action Key:**
- `sN` = Shift to state N
- `rN` = Reduce using Rule N
- `Acc` = Accept
- Empty = Error

**Rule Numbers:**
```
0: S' -> S
1: S  -> A
2: A  -> A b
3: A  -> a
```

### How Table Is Built

```cpp
for each state i:
    for each item in state i:
        if dot NOT at end and next symbol is terminal a:
            action[i][a] = "s" + next_state    // SHIFT
        if dot IS at end:
            if item is S' -> S.:
                action[i]["$"] = "Acc"         // ACCEPT
            else:
                for each terminal t:
                    action[i][t] = "r" + rule  // REDUCE (on ALL terminals!)

    for each non-terminal A:
        if GOTO(i, A) exists:
            goto[i][A] = next_state
```

> **Note:** LR(0) reduces on **ALL terminals** (no lookahead). This causes
> conflicts where multiple reductions apply to the same state/input.

---

## Step 4: Parsing Trace for `abb`

| State Stack | Symbol Stack | Input | Action |
|-------------|-------------|-------|--------|
| 0 | | abb$ | Shift a → s3 |
| 0 3 | a | bb$ | Reduce r3 (A→a) |
| 0 1 | A | bb$ | Shift b → s4 |
| 0 1 4 | A b | b$ | Reduce r2 (A→Ab) |
| 0 1 | A | b$ | Shift b → s4 |
| 0 1 4 | A b | $ | Reduce r2 (A→Ab) |
| 0 1 | A | $ | Reduce r1 (S→A) |
| 0 2 | S | $ | Accept! |

---

## Shift-Reduce Conflicts in LR(0)

This grammar has conflicts because LR(0) has **no lookahead**:

| State | Items | Conflict |
|-------|-------|----------|
| 1 | S -> A·, A -> A·b | Reduce r1 on $, Shift on b |
| 2 | S -> A·, A -> A·b | Reduce r1 on a,b,$, Shift on b |
| 3 | A -> a· | Reduce r3 on a,b,$ |
| 4 | A -> Ab· | Reduce r2 on a,b,$ |

**Resolution in practice:** Parser generators (yacc/bison) use precedence
or LALR(1) to resolve. For exam, just report the conflict.

---

## Code Structure (`lr0_parser.cpp`)

### Key Data Structures

```cpp
struct Rule {
    string lhs;           // "S", "A", "S'"
    vector<string> rhs;   // {"A"}, {"A","b"}, {"a"}
};

struct Item {
    int rule_idx;   // Which rule
    int dot_pos;    // Dot position (0 = before first symbol)
};
```

### Core Functions

```cpp
// Closure: expand dots before non-terminals
set<Item> closure(set<Item> I) {
    while (changed) {
        for each [A -> α·Bβ] in I:
            for each B -> γ:
                add [B -> ·γ] to I
    }
}

// GOTO: move dot past X, then closure
set<Item> goto_state(set<Item> I, string X) {
    J = {}
    for each item in I:
        if next symbol == X:
            J.insert(item with dot+1)
    return closure(J)
}

// Build all states
I0 = closure({S' -> ·S})
while new states added:
    for each state i:
        for each symbol X after dot:
            next = GOTO(i, X)
            if new: add as new state
            record transition (i, X) -> next
```

### Action/GOTO Table Construction

```cpp
for each state i:
    // SHIFT
    for each item with terminal after dot:
        action[i][terminal] = "s" + GOTO_state

    // REDUCE/ACCEPT
    for each item with dot at end:
        if rule == 0 (S' -> S.):
            action[i]["$"] = "Acc"
        else:
            for each terminal t:
                action[i][t] += "/r" + rule_idx

    // GOTO
    for each non-terminal A:
        if transition(i, A) exists:
            goto[i][A] = next_state
```

### Parsing Algorithm

```cpp
stack = [0]          // state stack
sym_stack = []       // symbol stack
input = tokens + "$"
ptr = 0

while true:
    state = stack.back()
    sym = input[ptr]
    act = action[state][sym]

    if act == "sN":
        stack.push(N)
        sym_stack.push(sym)
        ptr++
    else if act == "rN":
        rule = rules[N]
        pop 2*|RHS| from stack and sym_stack
        new_top = stack.back()
        sym_stack.push(rule.lhs)
        stack.push(goto[new_top][rule.lhs])
    else if act == "Acc":
        ACCEPT
    else:
        ERROR
```

---

## Exam Templates

### Minimal LR(0) Implementation (~80 lines)

```cpp
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>
using namespace std;

struct Rule { string lhs; vector<string> rhs; };
struct Item { int r, d; bool operator<(Item o) const { return r!=o.r?r<o.r:d<o.d; } };

vector<Rule> rules = { /* fill in */ };

set<Item> closure(set<Item> I) { /* as above */ }
set<Item> GOTO(set<Item> I, string X) { /* as above */ }

int main() {
    // 1. Build states
    // 2. Build tables
    // 3. Print table
    // 4. Parse input
}
```

### Memorization Checklist

- [ ] Item = (rule_idx, dot_pos)
- [ ] Closure: expand · before non-terminal
- [ ] GOTO: move dot past X, then closure
- [ ] I0 = closure({S'->·S})
- [ ] SHIFT: terminal after dot
- [ ] REDUCE: dot at end (all terminals!)
- [ ] ACCEPT: S'->S. with $
- [ ] GOTO table: for non-terminals after reduction
- [ ] Parse: stack of states, reduce pops 2*|RHS|

---

## Common Exam Variations

| Variation | What Changes |
|-----------|--------------|
| Different grammar | Recompute items, states, table |
| SLR(1) | Use FOLLOW to restrict reduce actions |
| LALR(1) | Merge compatible states from LR(1) |
| Conflict resolution | Precedence/associativity declarations |
| Error recovery | Panic mode, phrase level |
| Semantic actions | Attach code to reductions |

---

## Quick Reference

```
LR(0) ITEM:     Rule + dot position (A -> α·β)
CLOSURE(I):     Add B->·γ for every A->α·Bβ in I
GOTO(I, X):     Move dot past X in I, then closure
I0:             Closure({S'->·S})

ACTION TABLE:
  Shift:    if · before terminal a -> s(GOTO)
  Reduce:   if · at end (not S') -> rN on ALL terminals
  Accept:   if S'->S· with $ -> Acc
  Conflict: multiple entries in same cell

GOTO TABLE:   goto[s][A] = next state for non-terminal A

PARSE:
  stack=[0], input+= "$"
  loop:
    top, cur = stack.back(), input[ptr]
    if action[top][cur] = sN: push N, push cur, ptr++
    if action[top][cur] = rN:
       pop 2*|RHS|, push GOTO[new_top][LHS]
    if action[top][cur] = Acc: ACCEPT
    else ERROR

RULE NUMBERS:  0=S'->S, 1=S->A, 2=A->Ab, 3=A->a
```

---

## Compile & Run

```bash
# Full version
g++ lr0_parser.cpp -o lr0_parser
./lr0_parser

# Exam version (interactive)
g++ lr0_exam.cpp -o lr0_exam
./lr0_exam
```