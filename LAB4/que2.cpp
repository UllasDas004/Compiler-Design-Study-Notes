// ============================================================================
// QUESTION: Eliminate Left Recursion from a Grammar
// ============================================================================
//
// GRAMMAR (Left-Recursive):
// -------------------------
// E -> E + T | T
// T -> T * F | F
// F -> ( E ) | id
//
// PROBLEM:
// --------
// Left recursion (A -> A alpha) causes INFINITE RECURSION in LL(1) parsers
// and recursive descent parsers because A() calls A() calls A()...
// without ever consuming input.
//
// SOLUTION: Transform A -> A alpha | beta  into:
// --------------------------------------------
// A  -> beta A'
// A' -> alpha A' | eps
//
// Where:
//   - beta = non-recursive productions (don't start with A)
//   - alpha = recursive part (what comes after A in A -> A alpha)
//   - A' = new non-terminal (A-prime)
//
// EXAMPLE TRANSFORMATION:
// -----------------------
// Original: E -> E + T | T
//           E is left-recursive with alpha = "+ T", beta = "T"
//
// Transformed:
//   E  -> T E'
//   E' -> + T E' | eps
//
// Same for T:
//   T  -> F T'
//   T' -> * F T' | eps
// ============================================================================

#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace std;

// Type alias for grammar: Non-terminal -> list of productions (each production = vector of symbols)
using Grammar = map<string, vector<vector<string>>>;

// ============================================================================
// eliminateLeftRecursion: Removes direct left recursion from ALL non-terminals
// ============================================================================
// Algorithm for each non-terminal A:
//  1. Split productions into:
//     - recursive:    A -> A alpha  (starts with A)
//     - non-recursive: A -> beta    (doesn't start with A)
//  2. If no recursive productions: keep as-is
//  3. Otherwise create new non-terminal A' and rewrite:
//     - A  -> beta A'           (for each beta)
//     - A' -> alpha A'          (for each alpha)
//     - A' -> eps               (base case)
// ============================================================================
Grammar eliminateLeftRecursion(const Grammar& grammar) {
    Grammar result;

    // Process each non-terminal independently
    for (const auto& [nonTerminal, productions] : grammar) {
        vector<vector<string>> recursiveProductions;      // A -> A alpha
        vector<vector<string>> nonRecursiveProductions;   // A -> beta

        // ---- Step 1: Partition productions ----
        for (const auto& production : productions) {
            if (!production.empty() && production[0] == nonTerminal) {
                // Left-recursive: A -> A alpha
                // Extract alpha (everything after the first symbol)
                vector<string> alpha(production.begin() + 1, production.end());
                recursiveProductions.push_back(alpha);
            } else {
                // Non-recursive: A -> beta
                nonRecursiveProductions.push_back(production);
            }
        }

        // ---- Step 2: If no left recursion, copy as-is ----
        if (recursiveProductions.empty()) {
            result[nonTerminal] = productions;
            continue;
        }

        // ---- Step 3: Create new non-terminal A' ----
        string newNonTerminal = nonTerminal + "'";

        // A -> beta A'  (for each non-recursive production beta)
        for (const auto& beta : nonRecursiveProductions) {
            vector<string> newProduction = beta;
            newProduction.push_back(newNonTerminal);
            result[nonTerminal].push_back(newProduction);
        }

        // A' -> alpha A'  (for each recursive production alpha)
        for (const auto& alpha : recursiveProductions) {
            vector<string> newProduction = alpha;
            newProduction.push_back(newNonTerminal);
            result[newNonTerminal].push_back(newProduction);
        }

        // A' -> eps  (base case to terminate the recursion)
        result[newNonTerminal].push_back({"eps"});
    }

    return result;
}

// ============================================================================
// printGrammar: Pretty-prints the grammar in readable form
// ============================================================================
void printGrammar(const Grammar& grammar) {
    for (const auto& [nonTerminal, productions] : grammar) {
        cout << nonTerminal << " -> ";

        for (int i = 0; i < productions.size(); i++) {
            for (const string& symbol : productions[i]) {
                cout << symbol << " ";
            }

            if (i < productions.size() - 1) {
                cout << "| ";
            }
        }

        cout << "\n";
    }
}

int main() {
    // Original left-recursive grammar
    Grammar grammar = {
        {"E", {{"E", "+", "T"}, {"T"}}},
        {"T", {{"T", "*", "F"}, {"F"}}},
        {"F", {{"(", "E", ")"}, {"id"}}}
    };

    cout << "Original Grammar (Left-Recursive):\n";
    printGrammar(grammar);

    Grammar transformedGrammar = eliminateLeftRecursion(grammar);

    cout << "\nGrammar After Eliminating Left Recursion (LL(1)-compatible):\n";
    printGrammar(transformedGrammar);

    return 0;
}

// ============================================================================
// KEY CONCEPTS FOR EXAM:
// ============================================================================
//
// 1. LEFT RECURSION DEFINITION:
//    A -> A alpha  (non-terminal derives itself as FIRST symbol)
//    Example: E -> E + T
//
// 2. WHY IT BREAKS LL(1):
//    Recursive descent: E() calls E() calls E()... infinite loop
//    Table-driven:    FIRST(E) contains FIRST(E) -> circular
//
// 3. ELIMINATION ALGORITHM (DIRECT LEFT RECURSION ONLY):
//    Input:  A -> A alpha1 | A alpha2 | ... | beta1 | beta2 | ...
//    Output: A  -> beta1 A' | beta2 A' | ...
//            A' -> alpha1 A' | alpha2 A' | ... | eps
//
// 4. INDIRECT LEFT RECURSION:
//    A -> B alpha, B -> A beta
//    Requires ordering non-terminals and substituting (not covered here)
//
// 5. RESULT IS LL(1) COMPATIBLE:
//    The transformed grammar can be used to build FIRST/FOLLOW and
//    an LL(1) parsing table without conflicts.
//
// 6. EXAM VARIATIONS:
//    - Multiple recursive productions: A -> A a | A b | c
//    - Multiple non-recursive:       A -> A a | b | c
//    - Epsilon in beta:              A -> A a | eps  (special case)
//
// ============================================================================