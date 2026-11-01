// ============================================================================
// LR(0) PARSER - Complete Implementation with Detailed Comments
// ============================================================================
//
// GRAMMAR:
// --------
// S' -> S          (Rule 0: Augmented start)
// S  -> A          (Rule 1)
// A  -> A b        (Rule 2: Left-recursive)
// A  -> a          (Rule 3)
//
// INPUT STRING: "abb"
// EXPECTED PARSE: S => A => Ab => Abb => abb (wait, let's trace)
// Actually: S => A => Ab => a b => a b b? No...
// S -> A
// A -> A b | a
// For "abb": S => A => A b => A b b => a b b ✓
//
// LR(0) OVERVIEW:
// ---------------
// LR(0) = Left-to-right, Rightmost derivation in reverse, 0 lookahead
// Bottom-up parser: SHIFT tokens, REDUCE when RHS complete
// Uses ITEM SETS (states) to track parsing progress
// ============================================================================

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <iomanip>

using namespace std;

// ============================================================================
// DATA STRUCTURES
// ============================================================================

// RULE: Production with LHS (non-terminal) and RHS (vector of symbols)
struct Rule {
    string lhs;               // Left-hand side: "S", "A", "S'"
    vector<string> rhs;       // Right-hand side: {"A"} for S->A, {"A","b"} for A->Ab
};

// ITEM: LR(0) item = Rule index + dot position
// Example: Rule 2 (A -> A b), dot_pos=1  means  A -> A . b
struct Item {
    int rule_idx;      // Index into rules vector
    int dot_pos;       // Dot position (0 = before first symbol)

    // For set/map ordering
    bool operator<(const Item& other) const {
        if (rule_idx != other.rule_idx) return rule_idx < other.rule_idx;
        return dot_pos < other.dot_pos;
    }
    bool operator==(const Item& other) const {
        return rule_idx == other.rule_idx && dot_pos == other.dot_pos;
    }
};

// ============================================================================
// GRAMMAR DEFINITION (Hardcoded for this specific exam question)
// ============================================================================
vector<Rule> rules = {
    {"S'", {"S"}},      // Rule 0: S' -> S  (augmented start)
    {"S",  {"A"}},      // Rule 1: S  -> A
    {"A",  {"A", "b"}}, // Rule 2: A  -> A b
    {"A",  {"a"}}       // Rule 3: A  -> a
};

// ============================================================================
// CLOSURE OPERATION
// ============================================================================
// Given a set of items I, compute its closure:
// For each item [A -> alpha . B beta] in I,
// add all items [B -> . gamma] for every production B -> gamma
// Repeat until no new items added.
//
// WHY? When we've seen "alpha" and expect "B beta", we must also
// account for all possible ways to derive B (starting from its start).
// ============================================================================
set<Item> closure(set<Item> I) {
    set<Item> J = I;
    bool changed = true;

    while (changed) {
        changed = false;
        set<Item> temp = J;  // Copy to iterate safely while modifying J

        for (auto item : temp) {
            // If dot is NOT at the end, check symbol after dot
            if (item.dot_pos < rules[item.rule_idx].rhs.size()) {
                string next_sym = rules[item.rule_idx].rhs[item.dot_pos];

                // If next_sym is a non-terminal (has productions), expand it
                // Check if any rule has this as LHS
                for (size_t i = 0; i < rules.size(); ++i) {
                    if (rules[i].lhs == next_sym) {
                        Item new_item = {(int)i, 0};  // Dot at start of new rule
                        if (J.find(new_item) == J.end()) {
                            J.insert(new_item);
                            changed = true;
                        }
                    }
                }
            }
        }
    }
    return J;
}

// ============================================================================
// GOTO OPERATION
// ============================================================================
// GOTO(I, X) = Move dot past X in all items of I that have X after dot,
// then take closure of the result.
//
// This computes the state we reach after "seeing" symbol X in state I.
// ============================================================================
set<Item> goto_state(set<Item> I, string X) {
    set<Item> next_I;

    // For each item in I, if X is right after the dot, move dot past it
    for (auto item : I) {
        if (item.dot_pos < rules[item.rule_idx].rhs.size()) {
            if (rules[item.rule_idx].rhs[item.dot_pos] == X) {
                next_I.insert({item.rule_idx, item.dot_pos + 1});
            }
        }
    }

    // Take closure of the moved items
    return closure(next_I);
}

// ============================================================================
// MAIN: Build LR(0) Automaton and Parse
// ============================================================================
int main() {
    // ---- Step 1: Build all LR(0) states (item sets) ----
    vector<set<Item>> states;                    // All states (item sets)
    map<pair<int, string>, int> transitions;     // Transitions: (state, symbol) -> next_state

    // I0 = closure({S' -> .S})
    set<Item> I0 = closure({{0, 0}});
    states.push_back(I0);

    // Build states iteratively using GOTO
    bool changed = true;
    while (changed) {
        changed = false;

        // For each existing state
        for (size_t i = 0; i < states.size(); ++i) {
            // Collect all symbols that appear after a dot in this state
            set<string> symbols;
            for (auto item : states[i]) {
                if (item.dot_pos < rules[item.rule_idx].rhs.size()) {
                    symbols.insert(rules[item.rule_idx].rhs[item.dot_pos]);
                }
            }

            // For each such symbol, compute GOTO and add new state if needed
            for (auto X : symbols) {
                set<Item> next_I = goto_state(states[i], X);
                if (!next_I.empty()) {
                    // Check if this state already exists
                    int state_idx = -1;
                    for (size_t j = 0; j < states.size(); ++j) {
                        if (states[j] == next_I) {
                            state_idx = j;
                            break;
                        }
                    }

                    // If new state, add it
                    if (state_idx == -1) {
                        states.push_back(next_I);
                        state_idx = states.size() - 1;
                        changed = true;
                    }

                    // Record transition
                    transitions[{(int)i, X}] = state_idx;
                }
            }
        }
    }

    // ---- Step 2: Build ACTION and GOTO Tables ----
    map<int, map<string, string>> action_table;  // action[state][terminal] = "sN" or "rN" or "Acc"
    map<int, map<string, int>> goto_table;       // goto[state][nonterminal] = next_state

    // Initialize GOTO table with -1 (not present)
    for (size_t i = 0; i < states.size(); ++i) {
        goto_table[i]["S"] = -1;
        goto_table[i]["A"] = -1;
    }

    // Fill ACTION and GOTO tables
    for (size_t i = 0; i < states.size(); ++i) {
        // SHIFT: For each item with terminal after dot
        for (auto item : states[i]) {
            if (item.dot_pos < rules[item.rule_idx].rhs.size()) {
                string a = rules[item.rule_idx].rhs[item.dot_pos];
                // Terminals in this grammar: "a", "b"
                if (a == "a" || a == "b") {
                    action_table[i][a] = "s" + to_string(transitions[{i, a}]);
                }
            }
        }

        // REDUCE/ACCEPT: For each item with dot at end (complete RHS)
        for (auto item : states[i]) {
            if (item.dot_pos == rules[item.rule_idx].rhs.size()) {
                if (item.rule_idx == 0) {
                    // Rule 0: S' -> S.  -> ACCEPT on $
                    action_table[i]["$"] = "Acc";
                } else {
                    // Reduce: for ALL terminals (LR(0) has no lookahead!)
                    vector<string> terms = {"a", "b", "$"};
                    for (auto t : terms) {
                        if (action_table[i][t].empty()) {
                            action_table[i][t] = "r" + to_string(item.rule_idx);
                        } else {
                            // Multiple reductions possible = SHIFT/REDUCE CONFLICT
                            action_table[i][t] += "/r" + to_string(item.rule_idx);
                        }
                    }
                }
            }
        }

        // GOTO: For non-terminals
        for (auto nt : {"S", "A"}) {
            if (transitions.find({i, nt}) != transitions.end()) {
                goto_table[i][nt] = transitions[{i, nt}];
            }
        }
    }

    // ---- Step 3: Print Parsing Table ----
    cout << "LR(0) Parsing Table:\n";
    cout << left << setw(10) << "State"
         << setw(10) << "a" << setw(15)
         << "b" << setw(10) << "$"
         << setw(10) << "S" << setw(10)
         << "A" << "\n";
    cout << string(65, '-') << "\n";

    for (size_t i = 0; i < states.size(); ++i) {
        cout << left << setw(10) << i
             << setw(10) << (action_table[i]["a"].empty() ? "" : action_table[i]["a"])
             << setw(15) << (action_table[i]["b"].empty() ? "" : action_table[i]["b"])
             << setw(10) << (action_table[i]["$"].empty() ? "" : action_table[i]["$"])
             << setw(10) << (goto_table[i]["S"] == -1 ? "" : to_string(goto_table[i]["S"]))
             << setw(10) << (goto_table[i]["A"] == -1 ? "" : to_string(goto_table[i]["A"]))
             << "\n";
    }

    // ---- Step 4: Parse Input String "abb" ----
    cout << "\nTrace for input string 'abb':\n";
    vector<string> input = {"a", "b", "b", "$"};
    vector<int> states_stack = {0};       // State stack (starts with state 0)
    vector<string> sym_stack;              // Symbol stack (parallel to state stack)
    int ptr = 0;                           // Input pointer

    cout << left << setw(20) << "State Stack" << " | "
         << setw(20) << "Symbol Stack"
         << " | " << setw(15) << "Input"
         << " | Action\n";
    cout << string(85, '-') << "\n";

    while (true) {
        int state = states_stack.back();
        string sym = input[ptr];

        // Build stack strings for display
        string s_stack_str = "";
        for (auto s : states_stack) s_stack_str += to_string(s) + " ";

        string sym_stack_str = "";
        for (auto s : sym_stack) sym_stack_str += s + " ";

        string input_str = "";
        for (size_t i = ptr; i < input.size(); ++i) input_str += input[i];

        string act = action_table[state][sym];
        if (act.empty()) {
            cout << left << setw(20) << s_stack_str << " | "
            << setw(20) << sym_stack_str << " | "
            << setw(15) << input_str << " | Error\n";
            break;
        }

        // Handle conflicts: take first action (shift over reduce in this grammar)
        string actual_act = act;
        if (act.find('/') != string::npos) {
            actual_act = act.substr(0, act.find('/'));
        }

        if (actual_act[0] == 's') {
            // SHIFT: push new state and symbol, advance input
            cout << left << setw(20) << s_stack_str << " | "
            << setw(20) << sym_stack_str << " | "
            << setw(15) << input_str << " | Shift "
            << actual_act.substr(1) << "\n";

            states_stack.push_back(stoi(actual_act.substr(1)));
            sym_stack.push_back(sym);
            ptr++;
        }
        else if (actual_act[0] == 'r') {
            // REDUCE: pop |RHS| states/symbols, push LHS via GOTO
            int r_idx = stoi(actual_act.substr(1));
            Rule r = rules[r_idx];
            cout << left << setw(20)
            << s_stack_str << " | "
            << setw(20) << sym_stack_str << " | "
            << setw(15) << input_str << " | Reduce "
            << r_idx << " (" << r.lhs << " -> ";

            for (auto rs : r.rhs) cout << rs;
            cout << ")\n";

            // Pop RHS length states and symbols
            for (size_t i = 0; i < r.rhs.size(); ++i) {
                states_stack.pop_back();
                sym_stack.pop_back();
            }

            // Push LHS: new state = GOTO[top][LHS]
            int top_state = states_stack.back();
            sym_stack.push_back(r.lhs);
            states_stack.push_back(goto_table[top_state][r.lhs]);
        }
        else if (actual_act == "Acc") {
            // ACCEPT
            cout << left << setw(20)
                 << s_stack_str
                 << " | " << setw(20)
                 << sym_stack_str << " | " << setw(15)
                 << input_str << " | Accept\n";
            break;
        }
    }

    return 0;
}

// ============================================================================
// KEY CONCEPTS FOR EXAM:
// ============================================================================
//
// 1. LR(0) ITEMS:
//    - Production with dot: A -> alpha . beta
//    - Dot position = how much of RHS we've seen
//    - Rule index + dot position uniquely identifies an item
//
// 2. CLOSURE(I):
//    - For each item [A -> alpha . B beta] in I
//    - Add all [B -> . gamma] for every B-production
//    - Repeat until no new items
//    - Represents "what non-terminals we might be starting to parse"
//
// 3. GOTO(I, X):
//    - For each item in I with X after dot
//    - Move dot past X: [A -> alpha X . beta]
//    - Take closure of these moved items
//    - Represents "state after seeing X"
//
// 4. BUILDING STATES:
//    - I0 = closure({S' -> .S})
//    - For each state, for each symbol after dot: GOTO -> new state
//    - Repeat until no new states
//
// 5. ACTION TABLE:
//    - SHIFT: if terminal after dot -> action[s][a] = "sN"
//    - REDUCE: if dot at end (except S') -> action[s][t] = "rN" for ALL terminals t
//    - ACCEPT: if S' -> S. -> action[s]["$"] = "Acc"
//    - CONFLICT: multiple actions in same cell (LR(0) has many!)
//      State 2: S->A. (reduce r1) AND A->A.b (shift on b)
//      State 3: A->a. (reduce r3 on a,b,$)
//      State 4: A->Ab. (reduce r2 on a,b,$)
//
// 6. GOTO TABLE:
//    - goto[s][A] = next_state for non-terminal A
//    - Used after reduction: push GOTO[top][LHS]
//
// 7. PARSING ALGORITHM:
//    stack = [0], input = tokens + $
//    while true:
//      state = top of stack, sym = current input
//      if action[state][sym] = "sN": push N, push sym, advance input
//      if action[state][sym] = "rN":
//         pop 2*|RHS| items (states + symbols)
//         push LHS, push goto[new_top][LHS]
//      if action[state][sym] = "Acc": ACCEPT
//      else: ERROR
//
// 8. PARSE TRACE FOR "abb":
//    Stack    Symbols  Input   Action
//    0              abb$    Shift a -> s3
//    0 3            a       bb$    Reduce r3 (A->a) -> pop 1, push A, goto[0][A]=2
//    0 2            A       bb$    Shift b -> s4
//    0 2 4          Ab      b$    Reduce r2 (A->Ab) -> pop 2, push A, goto[2][A]=2
//    0 2            A       b$    Shift b -> s4
//    0 2 4          Ab      $     Reduce r2 (A->Ab) -> pop 2, push A, goto[2][A]=2
//    0 2            A       $     Reduce r1 (S->A) -> pop 1, push S, goto[0][S]=1
//    0 1            S       $     Accept!
//
// ============================================================================