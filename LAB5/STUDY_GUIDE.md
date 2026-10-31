# LAB 5 — LR(0) Parser

## Question

Build an LR(0) parsing table for the grammar below, then parse `abb`.

```
S  -> A
A  -> A b | a
```

(With augmented start: `S' -> S`)

---

## What is LR(0) Parsing?

- **L**eft-to-right scan, **R**ightmost derivation in reverse, **0** tokens of lookahead
- **Bottom-up** parsing: we **shift** tokens onto a stack and **reduce** when a right-hand side is complete
- Uses automaton states (Item sets) to decide Shift or Reduce

---

## Step 1: LR(0) Items

An **LR(0) item** is a production with a **dot** showing how much we've parsed:

```
S' -> .S      (haven't seen S yet)
S' -> S.      (have seen S -> ready to reduce to S')
A  -> .Ab     (haven't seen anything for A->Ab)
A  -> A.b     (seen A, expecting b)
A  -> Ab.     (seen Ab -> ready to reduce to A)
A  -> .a      (haven't seen a yet)
A  -> a.      (seen a -> ready to reduce to A)
```

---

## Step 2: Build Item Sets (States) via Closure & GOTO

### Closure Operation
Given an item set, for every item `[A -> alpha . B beta]`,
add all items `[B -> . gamma]` for every production of B.

### GOTO(I, X)
Move the dot past X in every item in I, then take closure.

### States for Our Grammar

**I0 (Initial State)**
```
S' -> .S
S  -> .A
A  -> .Ab
A  -> .a
```

**GOTO(I0, S) = I1**
```
S' -> S.      (accept state!)
```

**GOTO(I0, A) = I2**
```
S  -> A.
A  -> A.b
```

**GOTO(I0, a) = I3**
```
A  -> a.      (reduce state)
```

**GOTO(I2, b) = I4**
```
A  -> Ab.     (reduce state)
```

---

## Step 3: LR(0) Parsing Table

| State | a | b | $ | S | A |
|-------|---|---|---|---|---|
| 0 | s3 | | | 1 | 2 |
| 1 | | | Acc | | |
| 2 | r1,r2 | s4 | r1,r2 | | |
| 3 | r3 | r3 | r3 | | |
| 4 | r2 | r2 | r2 | | |

Where:
- `sN` = Shift to state N
- `rN` = Reduce using rule N
- `Acc` = Accept
- Numbers in GOTO columns = which state to go to after reduction

**Rules numbered:**
```
Rule 0: S' -> S
Rule 1: S  -> A
Rule 2: A  -> Ab
Rule 3: A  -> a
```

---

## Step 4: Parsing Trace for `abb`

| State Stack | Symbol Stack | Input | Action |
|-------------|-------------|-------|--------|
| 0 | | abb$ | Shift a -> go to s3 |
| 0 3 | a | bb$ | Reduce r3 (A->a) |
| 0 2 | A | bb$ | Shift b -> go to s4 |
| 0 2 4 | Ab | b$ | Reduce r2 (A->Ab) |
| 0 2 | A | b$ | Shift b -> go to s4 |
| 0 2 4 | Ab | $ | Reduce r2 (A->Ab) |
| 0 2 | A | $ | Reduce r1 (S->A) |
| 0 1 | S | $ | Accept! |

---

## Code Structure (`lr0_parser.cpp`)

### Key Data Structures
```cpp
struct Rule {
    string lhs;           // left-hand side: "S", "A", etc.
    vector<string> rhs;   // right-hand side: {"A","b"} for A->Ab
};

struct Item {
    int rule_idx;   // which rule
    int dot_pos;    // where the dot is (0 = before first symbol)
};
```

### closure() Function
```cpp
set<Item> closure(set<Item> I) {
    // For each item [A -> alpha . B beta] in I
    // Add all items [B -> . gamma] for each B-production
}
```

### goto_state() Function
```cpp
set<Item> goto_state(set<Item> I, string X) {
    // Move dot past X for all items in I that have X after the dot
    // Then take closure
}
```

### Building All States
```cpp
// Start with I0 = closure({S' -> .S})
// For each state, compute GOTO for every symbol after a dot
// New GOTO result = new state (if not already seen)
```

### Building Action/GOTO Tables
```cpp
for each state i:
    for each item in state i:
        if dot is NOT at end:
            if next symbol is terminal 'a':
                action[i]['a'] = "s" + to_string(next_state)  // Shift
        if dot IS at end:
            if item is S' -> S.:
                action[i]['$'] = "Acc"  // Accept
            else:
                for each terminal t:
                    action[i][t] = "r" + to_string(rule_idx)  // Reduce
```

---

## Shift-Reduce Conflict

In state 2 (containing both `S -> A.` and `A -> A.b`):
- When input is `b`: SHIFT to state 4 (because `A -> A.b` expects b)
- When input is `$`: REDUCE by `S -> A`

This is **not** a conflict because they're on different lookahead symbols.

---

## Exam Tips

- **Closure** = expand dots before non-terminals by adding their productions with dots at the start
- **GOTO(I, X)** = move dot past X, then closure
- **Shift** = terminal after dot -> push state onto stack, advance input
- **Reduce** = dot at end -> pop |RHS| states, push GOTO[top][LHS]
- **Accept** = reduce S' -> S. with `$` on input
- State with `S' -> S.` and `$` input = ACCEPT
