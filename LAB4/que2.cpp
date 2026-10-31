// Write a program to transform the following
// non-LL(1)/left-recursive grammar into an equivalent LL(1) form.

// E → E + T | T
// T → T * F | F
// F → ( E ) | id

#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace std;

using Grammar = map<string, vector<vector<string>>>;

Grammar eliminateLeftRecursion(const Grammar& grammar) {
    Grammar result;

    for (const auto& [nonTerminal, productions] : grammar) {
        vector<vector<string>> recursiveProductions;
        vector<vector<string>> nonRecursiveProductions;

        for (const auto& production : productions) {
            if (!production.empty() && production[0] == nonTerminal) {
                vector<string> alpha(production.begin() + 1, production.end());
                recursiveProductions.push_back(alpha);
            } else { 
                nonRecursiveProductions.push_back(production);
            }
        }

        if (recursiveProductions.empty()) {
            result[nonTerminal] = productions;
            continue;
        }

        string newNonTerminal = nonTerminal + "'";

        for (const auto& beta : nonRecursiveProductions) {
            vector<string> newProduction = beta;
            newProduction.push_back(newNonTerminal);
            result[nonTerminal].push_back(newProduction);
        }

        for (const auto& alpha : recursiveProductions) {
            vector<string> newProduction = alpha;
            newProduction.push_back(newNonTerminal);
            result[newNonTerminal].push_back(newProduction);
        }

        result[newNonTerminal].push_back({"eps"});
    }

    return result;
}

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
    Grammar grammar = {
        {"E", {{"E", "+", "T"}, {"T"}}},
        {"T", {{"T", "*", "F"}, {"F"}}},
        {"F", {{"(", "E", ")"}, {"id"}}}
    };

    cout << "Original Grammar:\n";
    printGrammar(grammar);

    Grammar transformedGrammar = eliminateLeftRecursion(grammar);

    cout << "\nGrammar After Eliminating Left Recursion:\n";
    printGrammar(transformedGrammar);

    return 0;
}
