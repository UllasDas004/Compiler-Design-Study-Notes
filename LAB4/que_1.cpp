// ============================================================================
// QUESTION: Construct LL(1) Parsing Table and Implement Table-Driven Predictive Parser
// ============================================================================
//
// GRAMMAR:
// --------
// S -> a A B e
// A -> b A c | d
// B -> f | epsilon
//
// CONCEPTS COVERED:
// -----------------
// 1. FIRST sets: What terminals can a non-terminal START with?
// 2. FOLLOW sets: What terminals can come AFTER a non-terminal?
// 3. Parsing Table: M[nonTerminal, terminal] = production to use
// 4. Table-Driven Parsing: Stack-based simulation of leftmost derivation
// ============================================================================

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <iomanip>

using namespace std;

// Type aliases for clarity
using Grammar       = map<string, vector<vector<string>>>;      // NT -> list of productions
using Sets          = map<string, set<string>>;                 // NT/Terminal -> set of terminals
using ParsingTable  = map<string, map<string, vector<string>>>; // M[NT][terminal] = production

// ============================================================================
// calculateFirst: Computes FIRST sets for all symbols
// ============================================================================
// FIRST(X) = set of terminals that can appear as FIRST symbol of string derived from X
//
// ALGORITHM (Iterative Fixed-Point):
// 1. Initialize: FIRST(terminal) = {terminal}, FIRST(eps) = {eps}
// 2. For each non-terminal A and production A -> X1 X2 ... Xk:
//    - Add FIRST(X1) - {eps} to FIRST(A)
//    - If eps in FIRST(X1), add FIRST(X2) - {eps}, etc.
//    - If ALL Xi can derive eps, add eps to FIRST(A)
// 3. Repeat until no changes (fixed point)
// ============================================================================
Sets calculateFirst(const Grammar& grammar) {
    Sets first;
    set<string> terminals;

    // ---- Step 1: Collect all terminals ----
    // A symbol is a terminal if it's NOT a non-terminal (not on LHS) and not eps
    for (const auto& [lhs, productions] : grammar) {
        for (const auto& production : productions) {
            for (const string& symbol : production) {
                if (!grammar.count(symbol) && symbol != "eps") {
                    terminals.insert(symbol);
                }
            }
        }
    }

    // ---- Step 2: Initialize FIRST for terminals ----
    // FIRST(a) = {a} for each terminal a
    for (const string& terminal : terminals) {
        first[terminal].insert(terminal);
    }
    first["eps"].insert("eps");  // epsilon derives epsilon

    // ---- Step 3: Iterative fixed-point computation ----
    bool changed = true;
    while (changed) {
        changed = false;

        for (const auto& [lhs, productions] : grammar) {
            for (const auto& production : productions) {
                bool nullable = true;  // Can entire production derive eps?

                for (const string& symbol : production) {
                    int oldSize = first[lhs].size();

                    // Add FIRST(symbol) - {eps} to FIRST(lhs)
                    for (const string& value : first[symbol]) {
                        if (value != "eps") {
                            first[lhs].insert(value);
                        }
                    }

                    if (first[lhs].size() != oldSize) {
                        changed = true;
                    }

                    // If symbol CANNOT derive eps, stop - later symbols don't matter
                    if (!first[symbol].count("eps")) {
                        nullable = false;
                        break;
                    }
                }

                // If ALL symbols in production can derive eps, then lhs can derive eps
                if (nullable) {
                    int oldSize = first[lhs].size();
                    first[lhs].insert("eps");
                    if (first[lhs].size() != oldSize) {
                        changed = true;
                    }
                }
            }
        }
    }

    return first;
}

// ============================================================================
// firstOfSequence: Computes FIRST of a sequence of symbols (production body)
// ============================================================================
// FIRST(X1 X2 ... Xk) = FIRST(X1) - {eps}
//   UNION (if eps in FIRST(X1)) FIRST(X2) - {eps}
//   UNION ...
//   UNION (if all nullable) {eps}
// ============================================================================
set<string> firstOfSequence(const vector<string>& sequence, const Sets& first) {
    set<string> result;
    bool nullable = true;

    for (const string& symbol : sequence) {
        if (symbol == "eps") {
            result.insert("eps");
            return result;
        }

        for (const string& value : first.at(symbol)) {
            if (value != "eps") {
                result.insert(value);
            }
        }

        if (!first.at(symbol).count("eps")) {
            nullable = false;
            break;
        }
    }

    if (nullable) {
        result.insert("eps");
    }

    return result;
}

// ============================================================================
// calculateFollow: Computes FOLLOW sets for all non-terminals
// ============================================================================
// FOLLOW(A) = set of terminals that can appear IMMEDIATELY AFTER A
// in some sentential form (valid prefix of derivation)
//
// RULES:
// 1. FOLLOW(startSymbol) includes $ (end of input)
// 2. If A -> alpha B beta:  add FIRST(beta) - {eps} to FOLLOW(B)
// 3. If A -> alpha B (B is last) OR beta can derive eps: add FOLLOW(A) to FOLLOW(B)
// ============================================================================
Sets calculateFollow(const Grammar& grammar, const Sets& first, const string& startSymbol) {
    Sets follow;

    // Initialize empty sets for all non-terminals
    for (const auto& [lhs, productions] : grammar) {
        follow[lhs];
    }

    // Rule 1: Start symbol gets $
    follow[startSymbol].insert("$");

    // Iterative fixed-point
    bool changed = true;
    while (changed) {
        changed = false;

        for (const auto& [lhs, productions] : grammar) {
            for (const auto& production : productions) {
                for (int i = 0; i < production.size(); i++) {
                    const string& symbol = production[i];

                    // Only compute FOLLOW for non-terminals
                    if (!grammar.count(symbol)) {
                        continue;
                    }

                    // Remaining symbols after 'symbol' in this production
                    vector<string> remaining(production.begin() + i + 1, production.end());
                    set<string> firstRemaining = firstOfSequence(remaining, first);

                    int oldSize = follow[symbol].size();

                    // Rule 2: Add FIRST(remaining) - {eps}
                    for (const string& value : firstRemaining) {
                        if (value != "eps") {
                            follow[symbol].insert(value);
                        }
                    }

                    // Rule 3: If remaining is empty OR can derive eps, add FOLLOW(lhs)
                    if (remaining.empty() || firstRemaining.count("eps")) {
                        follow[symbol].insert(follow[lhs].begin(), follow[lhs].end());
                    }

                    if (follow[symbol].size() != oldSize) {
                        changed = true;
                    }
                }
            }
        }
    }

    return follow;
}

// ============================================================================
// buildParsingTable: Constructs LL(1) parsing table M[NT][terminal]
// ============================================================================
// For each production A -> alpha:
//   For each terminal a in FIRST(alpha):
//       M[A, a] = alpha
//   If eps in FIRST(alpha):
//       For each b in FOLLOW(A):
//           M[A, b] = alpha
//
// CONFLICT CHECK: If M[A, a] already has an entry -> NOT LL(1)
// ============================================================================
ParsingTable buildParsingTable(const Grammar& grammar, const Sets& first, const Sets& follow) {
    ParsingTable table;

    for (const auto& [lhs, productions] : grammar) {
        for (const auto& production : productions) {
            set<string> firstProduction = firstOfSequence(production, first);

            // Case 1: For each terminal in FIRST(production)
            for (const string& terminal : firstProduction) {
                if (terminal != "eps") {
                    table[lhs][terminal] = production;
                }
            }

            // Case 2: If eps in FIRST(production), use FOLLOW(lhs)
            if (firstProduction.count("eps")) {
                for (const string& terminal : follow.at(lhs)) {
                    table[lhs][terminal] = production;
                }
            }
        }
    }

    return table;
}

// ============================================================================
// printSet: Prints FIRST or FOLLOW sets (only for non-terminals)
// ============================================================================
void printSet(const string& name, const Sets& sets) {
    for (const auto& [symbol, values] : sets) {
        // Skip terminals (lowercase) and eps
        if (symbol == "eps" || !isupper(symbol[0])) {
            continue;
        }

        cout << name << "(" << symbol << ") = { ";

        for (const string& value : values) {
            cout << value << " ";
        }

        cout << "}\n";
    }
}

// ============================================================================
// parse: Table-driven LL(1) predictive parser
// ============================================================================
// DATA STRUCTURES:
//   - stack: starts with [$, startSymbol] (top at back)
//   - input: tokens + $ at end
//
// ALGORITHM:
//   while stack not empty:
//     top = stack.back()
//     current = input[pointer]
//
//     if top == current == '$':  ACCEPT
//     if top == current:         MATCH -> pop, advance
//     if M[top][current] exists: EXPAND -> pop, push production in REVERSE
//     else:                      ERROR
// ============================================================================
bool parse(const vector<string>& tokens, const ParsingTable& table, const string& startSymbol) {
    vector<string> input = tokens;
    input.push_back("$");

    vector<string> stack = {"$", startSymbol};
    int pointer = 0;

    cout << "\n";
    cout<< left << setw(20) << "Stack"
        << setw(20) << "Input"
        << "Action\n";
    cout << string(60, '-') << "\n";

    while (!stack.empty()) {
        string stackString;
        for (const string& symbol : stack) {
            stackString += symbol;
        }

        string inputString;
        for (int i = pointer; i < input.size(); i++) {
            inputString += input[i];
        }

        string top = stack.back();
        string current = input[pointer];

        // Accept condition
        if (top == "$" && current == "$") {
            cout<< left << setw(20) << stackString
                << setw(20) << inputString
                << "Accept\n";
            return true;
        }

        // Match terminal
        if (top == current) {
            cout<< left << setw(20) << stackString
                << setw(20) << inputString
                << "Match " << top << "\n";

            stack.pop_back();
            pointer++;
        }
        // Expand non-terminal using parsing table
        else if (table.count(top) && table.at(top).count(current)) {
            vector<string> production = table.at(top).at(current);

            string productionString;
            for (const string& symbol : production) {
                productionString += symbol;
            }

            cout<< left << setw(20) << stackString
                << setw(20) << inputString
                << top << " -> " << productionString << "\n";

            stack.pop_back();

            // Push production in REVERSE (so first symbol ends up on top)
            if (!(production.size() == 1 && production[0] == "eps")) {
                for (int i = production.size() - 1; i >= 0; i--) {
                    stack.push_back(production[i]);
                }
            }
        }
        // Error: no table entry
        else {
            cout<< left << setw(20) << stackString
                << setw(20) << inputString
                << "Error\n";

            return false;
        }
    }

    return false;
}

int main() {
    // Grammar: S -> a A B e, A -> b A c | d, B -> f | eps
    Grammar grammar = {
        {"S", {{"a", "A", "B", "e"}}},
        {"A", {{"b", "A", "c"}, {"d"}}},
        {"B", {{"f"}, {"eps"}}}
    };

    string startSymbol = "S";

    // Step 1: Compute FIRST sets
    Sets first = calculateFirst(grammar);

    // Step 2: Compute FOLLOW sets
    Sets follow = calculateFollow(grammar, first, startSymbol);

    // Step 3: Build LL(1) parsing table
    ParsingTable table = buildParsingTable(grammar, first, follow);

    // Output results
    cout << "FIRST Sets\n";
    printSet("FIRST", first);

    cout << "\nFOLLOW Sets\n";
    printSet("FOLLOW", follow);

    cout << "\nLL(1) Parsing Table\n";

    for (const auto& [nonTerminal, entries] : table) {
        for (const auto& [terminal, production] : entries) {
            cout<< "M[" << nonTerminal << ", " << terminal << "] = "
                << nonTerminal << " -> ";

            for (const string& symbol : production) {
                cout << symbol;
            }

            cout << "\n";
        }
    }

    // Step 4: Parse input "adfe" (corresponds to: a d f e)
    vector<string> input = {"a", "d", "f", "e"};

    cout << "\nParsing input: adfe\n";

    parse(input, table, startSymbol);

    return 0;
}

// ============================================================================
// STEP-BY-STEP WALKTHROUGH FOR THIS GRAMMAR:
// ============================================================================
//
// GRAMMAR:
// S -> a A B e
// A -> b A c | d
// B -> f | eps
//
// FIRST SETS:
// -----------
// FIRST(a) = {a}, FIRST(b) = {b}, FIRST(c) = {c}, FIRST(d) = {d}, FIRST(e) = {e}, FIRST(f) = {f}
// FIRST(eps) = {eps}
//
// FIRST(B):
//   B -> f        => {f}
//   B -> eps      => {eps}
//   FIRST(B) = {f, eps}
//
// FIRST(A):
//   A -> b A c    => FIRST(b) = {b}
//   A -> d        => {d}
//   FIRST(A) = {b, d}
//
// FIRST(S):
//   S -> a A B e  => FIRST(a) = {a}
//   FIRST(S) = {a}
//
// FOLLOW SETS:
// ------------
// FOLLOW(S) = {$}                    (Rule 1: start symbol)
//
// FOLLOW(A):
//   In S -> a A B e: after A comes B
//   FIRST(B) = {f, eps}
//   Add {f} to FOLLOW(A) (Rule 2)
//   Since eps in FIRST(B), add FOLLOW(S) = {$} to FOLLOW(A) (Rule 3)
//   In A -> b A c: after A comes c => add {c} (Rule 2)
//   FOLLOW(A) = {f, c, $}
//
// FOLLOW(B):
//   In S -> a A B e: after B comes e => add {e} (Rule 2)
//   FOLLOW(B) = {e}
//
// PARSING TABLE:
// --------------
// M[S, a] = a A B e
// M[A, b] = b A c
// M[A, d] = d
// M[A, f] = eps? NO! B -> eps goes in M[B, e]
// M[A, c] = ? No - c not in FOLLOW(A) wait... FOLLOW(A) = {f, c, $}
// Actually for A -> d: FIRST(d) = {d} -> M[A, d] = d
// For A -> b A c: FIRST(b A c) = {b} -> M[A, b] = b A c
// M[B, f] = f
// M[B, e] = eps  (since eps in FIRST(B), and e in FOLLOW(B))
//
// INPUT: a d f e
// STACK TRACE:
// $S          a d f e$   S -> a A B e
// $e B A a    a d f e$   Match a
// $e B A      d f e$     A -> d
// $e B        d f e$     Match d
// $e B        f e$       B -> f
// $e          f e$       Match f
// $e          e$         Match e
// $           $          Accept
//
// ============================================================================
// EXAM TEMPLATES:
// ============================================================================
//
// FIRST SET CALCULATION:
// ---------------------
// Sets first;
// for terminals: first[t].insert(t);
// first["eps"].insert("eps");
// while(changed) {
//   for each production A -> X1 X2 ... Xk:
//     nullable = true
//     for each Xi:
//       add first[Xi] - {eps} to first[A]
//       if eps not in first[Xi]: nullable = false; break
//     if nullable: add eps to first[A]
// }
//
// FOLLOW SET CALCULATION:
// ----------------------
// follow[start].insert("$");
// while(changed) {
//   for each production A -> X1 X2 ... Xk:
//     for i = 0 to k-1:
//       if Xi is non-terminal:
//         remaining = Xi+1 ... Xk
//         firstRem = firstOfSequence(remaining)
//         add firstRem - {eps} to follow[Xi]
//         if remaining empty or eps in firstRem:
//           add follow[A] to follow[Xi]
// }
//
// PARSING TABLE:
// --------------
// for each A -> alpha:
//   for a in FIRST(alpha) - {eps}: M[A,a] = alpha
//   if eps in FIRST(alpha):
//     for b in FOLLOW(A): M[A,b] = alpha
//
// TABLE-DRIVEN PARSER:
// --------------------
// stack = {$, start}
// input = tokens + $
// while stack not empty:
//   top = stack.top()
//   cur = input[ptr]
//   if top == cur == $: accept
//   if top == cur: pop, ptr++
//   else if M[top][cur] exists:
//     pop
//     push production in reverse (skip eps)
//   else: error
//
// ============================================================================