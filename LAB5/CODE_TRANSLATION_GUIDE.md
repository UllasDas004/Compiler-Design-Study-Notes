# From Paper to Code: LR(0) Parser Translation Guide
## How to Turn Algorithm Steps into C++ Data Structures & Loops

---

## The Core Insight

| Paper Concept | Code Representation |
|---------------|---------------------|
| **Grammar Rule** | `struct Rule { string lhs; vector<string> rhs; }` |
| **LR(0) Item** | `struct Item { int rule_idx; int dot_pos; }` |
| **Item Set (State)** | `set<Item>` (or `vector<Item>`) |
| **All States** | `vector<set<Item>> states` |
| **Closure(I)** | Function returning `set<Item>` |
| **GOTO(I, X)** | Function returning `set<Item>` |
| **DFA Transitions** | `map<pair<int,string>, int>` |
| **Action Table** | `map<int, map<string, string>>` |
| **Goto Table** | `map<int, map<string, int>>` |

---

## Step-by-Step Translation

### 1. Grammar → Data Structure

**On Paper:**
```
Rule 0: S' → S
Rule 1: S  → A
Rule 2: A  → A b
Rule 3: A  → a
```

**In Code:**
```cpp
struct Rule {
    string lhs;              // "S", "A", "S'"
    vector<string> rhs;      // {"S"}, {"A"}, {"A","b"}, {"a"}
};

vector<Rule> rules = {
    {"S'", {"S"}},
    {"S",  {"A"}},
    {"A",  {"A", "b"}},
    {"A",  {"a"}}
};
```

**Key Decision:** Store RHS as `vector<string>` because we need to access symbols by index (dot position).

---

### 2. LR(0) Item → Struct

**On Paper:** `A → A · b` (Rule 2, dot after first symbol)

**In Code:**
```cpp
struct Item {
    int rule_idx;    // 2 (which rule)
    int dot_pos;     // 1 (dot after 1 symbol)
    
    // For set/map ordering
    bool operator<(const Item& o) const {
        return rule_idx != o.rule_idx ? rule_idx < o.rule_idx : dot_pos < o.dot_pos;
    }
    bool operator==(const Item& o) const {
        return rule_idx == o.rule_idx && dot_pos == o.dot_pos;
    }
};
```

**Why these fields?** 
- `rule_idx` → look up the rule in `rules` vector
- `dot_pos` → index into `rules[rule_idx].rhs`

---

### 3. Closure(I) → Function

**On Paper Algorithm:**
```
CLOSURE(I):
    J = I
    repeat
        for each [A → α·Bβ] in J:
            for each B → γ:
                add [B → ·γ] to J
    until no new items added
    return J
```

**Translation to Code - Line by Line:**

```cpp
set<Item> closure(set<Item> I) {
    set<Item> J = I;                    // J = I
    bool changed = true;
    
    while (changed) {                   // repeat
        changed = false;
        set<Item> temp = J;             // iterate over snapshot (safe modification)
        
        for (auto item : temp) {        // for each item in J
            // Check if dot NOT at end: item.dot_pos < rhs.size()
            if (item.dot_pos < rules[item.rule_idx].rhs.size()) {
                string B = rules[item.rule_idx].rhs[item.dot_pos];  // symbol after dot
                
                // for each B → γ:
                for (int i = 0; i < rules.size(); i++) {
                    if (rules[i].lhs == B) {          // found B-production
                        Item new_item = {i, 0};       // [B → ·γ] dot at 0
                        if (J.insert(new_item).second) { // if newly added
                            changed = true;           // mark for another pass
                        }
                    }
                }
            }
        }
    }
    return J;                           // return J
}
```

**Critical Pattern:** `while (changed)` loop = "repeat until fixed point"

---

### 4. GOTO(I, X) → Function

**On Paper:**
```
GOTO(I, X):
    J = {}
    for each [A → α·Xβ] in I:
        add [A → αX·β] to J
    return CLOSURE(J)
```

**In Code:**
```cpp
set<Item> goto_state(set<Item> I, string X) {
    set<Item> J;                        // J = {}
    
    for (auto item : I) {               // for each item in I
        // Check if X is right after dot
        if (item.dot_pos < rules[item.rule_idx].rhs.size() &&
            rules[item.rule_idx].rhs[item.dot_pos] == X) {
            
            // Move dot past X: dot_pos + 1
            J.insert({item.rule_idx, item.dot_pos + 1});
        }
    }
    return closure(J);                  // return CLOSURE(J)
}
```

---

### 5. Building All States = DFA Construction

**On Paper:**
```
states = {CLOSURE({[S' → ·S]})}
repeat
    for each state I in states:
        for each symbol X after a dot in I:
            J = GOTO(I, X)
            if J not in states:
                add J to states
until no new states
```

**In Code - The Exact Same Algorithm:**
```cpp
vector<set<Item>> states;
map<pair<int, string>, int> transitions;  // (state_id, symbol) -> next_state_id

// Start state I0 = CLOSURE({[S' → ·S]})
states.push_back(closure({{0, 0}}));

bool changed = true;
while (changed) {
    changed = false;
    
    for (size_t i = 0; i < states.size(); i++) {
        // Collect all symbols after dots in this state
        set<string> symbols;
        for (auto item : states[i]) {
            if (item.dot_pos < rules[item.rule_idx].rhs.size()) {
                symbols.insert(rules[item.rule_idx].rhs[item.dot_pos]);
            }
        }
        
        // For each symbol, compute GOTO
        for (auto X : symbols) {
            set<Item> next = goto_state(states[i], X);
            if (next.empty()) continue;
            
            // Check if already exists
            int idx = -1;
            for (size_t j = 0; j < states.size(); j++) {
                if (states[j] == next) { idx = j; break; }
            }
            
            // If new, add it
            if (idx == -1) {
                states.push_back(next);
                idx = states.size() - 1;
                changed = true;
            }
            
            // Record transition
            transitions[{(int)i, X}] = idx;
        }
    }
}
```

**Key Insight:** This `while (changed)` loop IS the DFA construction. Each `states[i]` is a DFA node. `transitions` is the edge list.

---

### 6. Building ACTION Table

**On Paper:**
```
for each state i:
    for each item in i:
        if item = [A → α·aβ] (a is terminal):
            ACTION[i, a] = shift GOTO(i, a)
        if item = [A → α·] (dot at end):
            if A = S': ACTION[i, $] = accept
            else: for each terminal t: ACTION[i, t] = reduce A→α
```

**In Code:**
```cpp
map<int, map<string, string>> action;

for (size_t i = 0; i < states.size(); i++) {
    // SHIFT: terminal after dot
    for (auto item : states[i]) {
        if (item.dot_pos < rules[item.rule_idx].rhs.size()) {
            string a = rules[item.rule_idx].rhs[item.dot_pos];
            if (is_terminal(a)) {                    // "a" or "b" in our grammar
                action[i][a] = "s" + to_string(transitions[{i, a}]);
            }
        }
    }
    
    // REDUCE/ACCEPT: dot at end
    for (auto item : states[i]) {
        if (item.dot_pos == rules[item.rule_idx].rhs.size()) {
            if (item.rule_idx == 0) {                // S' → S·
                action[i]["$"] = "Acc";
            } else {
                // LR(0): reduce on ALL terminals
                for (auto t : {"a", "b", "$"}) {
                    if (action[i][t].empty())
                        action[i][t] = "r" + to_string(item.rule_idx);
                    else
                        action[i][t] += "/r" + to_string(item.rule_idx); // conflict
                }
            }
        }
    }
}
```

---

### 7. Building GOTO Table

**On Paper:**
```
for each state i:
    for each non-terminal A:
        if GOTO(i, A) exists:
            GOTO[i, A] = that state
```

**In Code:**
```cpp
map<int, map<string, int>> goto_table;

for (size_t i = 0; i < states.size(); i++) {
    for (auto A : {"S", "A"}) {      // all non-terminals
        if (transitions.count({i, A})) {
            goto_table[i][A] = transitions[{i, A}];
        }
    }
}
```

---

### 8. Parsing Loop

**On Paper Algorithm:**
```
stack = [0]
input = tokens + $
while true:
    s = top(stack), a = current input
    if ACTION[s, a] = shift t:
        push t, push a, advance input
    if ACTION[s, a] = reduce A→β:
        pop 2*|β| items from stack
        s' = top(stack)
        push A, push GOTO[s', A]
    if ACTION[s, a] = accept: success
    else: error
```

**In Code:**
```cpp
vector<int> state_stack = {0};       // state stack
vector<string> symbol_stack;          // parallel symbol stack
int ptr = 0;                          // input pointer

while (true) {
    int s = state_stack.back();
    string a = input[ptr];
    
    string act = action[s][a];
    
    if (act[0] == 's') {                     // SHIFT
        int t = stoi(act.substr(1));
        state_stack.push_back(t);
        symbol_stack.push_back(a);
        ptr++;
    }
    else if (act[0] == 'r') {                // REDUCE
        int rule_idx = stoi(act.substr(1));
        Rule r = rules[rule_idx];
        
        // Pop |RHS| states AND symbols
        for (int i = 0; i < r.rhs.size(); i++) {
            state_stack.pop_back();
            symbol_stack.pop_back();
        }
        
        // Push LHS via GOTO
        int new_top = state_stack.back();
        symbol_stack.push_back(r.lhs);
        state_stack.push_back(goto_table[new_top][r.lhs]);
    }
    else if (act == "Acc") break;            // ACCEPT
    else break;                               // ERROR
}
```

**Why pop 2*|RHS|?** Each grammar symbol pushed also pushed a state. So |RHS| symbols = |RHS| states on stack.

---

## Complete Minimal Implementation Template

```cpp
#include <bits/stdc++.h>
using namespace std;

struct Rule { string lhs; vector<string> rhs; };
struct Item { int r, d; bool operator<(Item o) const { return r!=o.r?r<o.r:d<o.d;} } };

vector<Rule> rules = { /* your grammar */ };

set<Item> closure(set<Item> I) { /* as above */ }
set<Item> GOTO(set<Item> I, string X) { /* as above */ }

int main() {
    // 1. Build states
    vector<set<Item>> states = {closure({{0,0}})};
    map<pair<int,string>,int> trans;
    // ... while(changed) loop ...
    
    // 2. Build tables
    // ... action and goto ...
    
    // 3. Parse
    // ... while(true) loop ...
}
```

---

## Practice Exercise: Build It From Scratch

1. **Start with just the grammar struct** - compile
2. **Add Item struct + closure** - test with I0
3. **Add GOTO** - test GOTO(I0, 'a')
4. **Add state building loop** - print all states
5. **Add table construction** - print table
6. **Add parser** - test with input

**Test after each step.** Don't write 200 lines then compile.

---

## Mental Checklist When Writing

- [ ] `Item` has `rule_idx` + `dot_pos` + comparison ops
- [ ] `closure` uses `while(changed)` fixed-point
- [ ] `GOTO` moves dot then calls `closure`
- [ ] State building uses `transitions` map for edges
- [ ] SHIFT on terminal after dot
- [ ] REDUCE on dot-at-end (all terminals for LR(0))
- [ ] ACCEPT on `S' → S·` with `$`
- [ ] GOTO table for non-terminals
- [ ] Parser pops 2*|RHS|, pushes LHS via GOTO