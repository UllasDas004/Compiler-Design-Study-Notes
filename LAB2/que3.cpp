// ============================================================================
// QUESTION: Write a program to generate a suitable CFG
// and the language pattern generated using the CFG in terms of n where n>0,
// for the following input of terminals: aabaabaa.
// ============================================================================
//
// WHAT IS A CONTEXT-FREE GRAMMAR (CFG)?
// -------------------------------------
// A CFG is a set of production rules that describe all possible strings
// in a given formal language. It consists of:
//   - Terminals: The actual symbols of the language (e.g., 'a', 'b')
//   - Non-terminals: Variables that can be replaced (e.g., S, A, B)
//   - Productions: Rules of the form NonTerminal -> sequence of terminals/non-terminals
//   - Start Symbol: The non-terminal from which derivation begins (usually S)
//
// CFGs are more powerful than regular expressions - they can describe
// languages with nested/recursive structure (like balanced parentheses).
//
// ANALYZING THE STRING: aabaabaa
// -------------------------------
// Let's break it down visually:
//
// aabaabaa
// ^^^        = "aab"  (first repeating block)
//    ^^^     = "aab"  (second repeating block)
//       ^^   = "aa"   (terminating tail)
//
// So the pattern is:  (aab) repeated n times, followed by "aa"
// Where n = 2 for this specific string.
//
// GENERAL LANGUAGE PATTERN:
// L = { (aab)^n aa | n > 0 }
//
// This generates:  aa, aabaab, aabaabaab, aabaabaabaab, ...
//                  n=1     n=2       n=3           n=4
//
// Note: n > 0 means at least ONE "aab" block before the final "aa".
// If n=0 were allowed, the language would include just "aa".
// ============================================================================
//
// CFG DESIGN:
// -----------
// We want a grammar that generates exactly: (aab)^n aa for n > 0
//
// S -> aab S | aa
//
// This means:
//   S can produce "aab" followed by another S (recursive case - add another block)
//   S can produce "aa" (base case - the terminating tail)
//
// This is a RIGHT-LINEAR grammar (all productions have non-terminal at the end).
// Right-linear grammars generate regular languages.
// Indeed, this language IS regular (can be expressed as regex: (aab)+aa).
// But the question asks for a CFG, and this CFG works perfectly.
// ============================================================================
//
// DERIVATION FOR n=2 (the string aabaabaa):
// ------------------------------------------
// S
// => aabS          (apply S -> aabS)
// => aabaabS       (apply S -> aabS again, now we have 2 "aab" blocks)
// => aabaabaa      (apply S -> aa, the base case)
//
// Each step applies ONE production rule. The derivation shows HOW
// the start symbol S expands to the final string.
// ============================================================================
//
// WHY THIS CFG IS CORRECT:
// ------------------------
// For n=1: S => aabS => aabaab  (one "aab" + "aa")
// For n=2: S => aabS => aabaabS => aabaabaa  (two "aab" + "aa")
// For n=3: S => aabS => aabaabS => aabaabaabS => aabaabaabaa
//
// The grammar generates exactly the language { (aab)^n aa | n > 0 }
// ============================================================================
//
// KEY CONCEPTS TO REMEMBER FOR EXAM:
// ----------------------------------
// 1. IDENTIFY THE PATTERN: Look for repeating units and a tail
// 2. WRITE THE LANGUAGE FORMULA: L = { (repeating_unit)^n tail | n > 0 }
// 3. CREATE THE CFG: S -> repeating_unit S | tail
// 4. SHOW DERIVATION: Step-by-step application of productions
// 5. VERIFY: Count n from the given string, trace the derivation
// ============================================================================

#include <iostream>
#include <string>

using namespace std;


// ============================================================================
// generateCFG(): Analyzes the input string, identifies the pattern,
// prints the CFG, and shows the step-by-step derivation.
// ============================================================================
void generateCFG(const string& input) {
    // The repeating unit we identified from manual analysis
    string repeating_unit = "aab";
    // The terminating tail
    string tail = "aa";

    cout << "Analyzing string: " << input << "\n";
    cout << "--------------------------------------------------\n";

    // ---- STEP 1: Language Pattern Analysis ----
    cout << "1. Language Pattern:\n";
    cout << "   The string consists of the repeating block '" << repeating_unit
         << "' followed by a terminating '" << tail << "'.\n";
    cout << "   Pattern formula: L = { (" << repeating_unit << ")^n " << tail << " | n > 0 }\n";

    // Calculate n from the string length
    // Total length = n * len(repeating_unit) + len(tail)
    // So n = (total_len - tail_len) / repeating_unit_len
    int n = (input.length() - tail.length()) / repeating_unit.length();
    cout << "   For the given input '" << input << "', n = " << n << ".\n\n";

    // ---- STEP 2: The CFG ----
    cout << "2. Context-Free Grammar (CFG):\n";
    cout << "   S -> " << repeating_unit << "S | " << tail << "\n\n";

    // ---- STEP 3: Derivation ----
    cout << "3. Derivation for n = " << n << ":\n";
    string current = "S";
    cout << "   " << current << "\n";

    // Apply the recursive rule n times: S -> aabS
    for (int i = 0; i < n; i++) {
        // Replace the single 'S' with 'aabS'
        current = current.replace(current.find("S"), 1, repeating_unit + "S");
        cout << "   => " << current << "\n";
    }

    // Apply the base case: S -> aa
    current = current.replace(current.find("S"), 1, tail);
    cout << "   => " << current << " (Final derived string)\n";
}


// ============================================================================
// main(): Entry point - just calls the analysis function
// ============================================================================
int main() {
    string terminals = "aabaabaa";
    generateCFG(terminals);
    return 0;
}


// ============================================================================
// EXAM TEMPLATE FOR CFG QUESTIONS:
// --------------------------------
// Given a string, follow these steps:
//
// 1. PATTERN IDENTIFICATION:
//    Write out the string and group characters to find repeating unit + tail
//    Example: abbabbabbc
//             ^^^ ^^^ ^^^ ^
//             abb abb abb c
//             Unit: "abb", Tail: "c", n=3
//
// 2. LANGUAGE FORMULA:
//    L = { (unit)^n tail | n > 0 }
//    L = { (abb)^n c | n > 0 }
//
// 3. CFG:
//    S -> unit S | tail
//    S -> abb S | c
//
// 4. DERIVATION (for given n):
//    S
//    => abbS
//    => abbabbS
//    => abbabbabbS
//    => abbabbabbc
//
// 5. KEY INSIGHT: If string = unit^n tail, grammar is S -> unit S | tail
//    This is ALWAYS the pattern for this type of question.
// ============================================================================