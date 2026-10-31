// Write a program to construct an LL(1)
// parsing table for a given grammar and
// implement a table-driven predictive parser.

// S → a A B e
// A → b A c | d
// B → f | ε

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <iomanip>

using namespace std;

using Grammar = map<string, vector<vector<string>>>;
using Sets = map<string, set<string>>;
using ParsingTable = map<string, map<string, vector<string>>>;


Sets calculateFirst(const Grammar& grammar) {
    Sets first;
    set<string> terminals;

    for (const auto& [lhs, productions] : grammar) {
        for (const auto& production : productions) {
            for (const string& symbol : production) {
                if (!grammar.count(symbol) && symbol != "eps") {
                    terminals.insert(symbol); 
                }
            }
        }
    }

    for (const string& terminal : terminals) {
        first[terminal].insert(terminal);
    }

    first["eps"].insert("eps");

    bool changed = true;

    while (changed) {
        changed = false;

        for (const auto& [lhs, productions] : grammar) {
            for (const auto& production : productions) {
                bool nullable = true;

                for (const string& symbol : production) {
                    int oldSize = first[lhs].size();

                    for (const string& value : first[symbol]) {
                        if (value != "eps") {
                            first[lhs].insert(value);
                        }
                    }

                    if (first[lhs].size() != oldSize) {
                        changed = true;
                    }

                    if (!first[symbol].count("eps")) {
                        nullable = false;
                        break;
                    }
                }

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

Sets calculateFollow(const Grammar& grammar, const Sets& first, const string& startSymbol) {
    Sets follow;

    for (const auto& [lhs, productions] : grammar) {
        follow[lhs];
    }

    follow[startSymbol].insert("$");

    bool changed = true;

    while (changed) {
        changed = false;

        for (const auto& [lhs, productions] : grammar) {
            for (const auto& production : productions) {
                for (int i = 0; i < production.size(); i++) {
                    const string& symbol = production[i];

                    if (!grammar.count(symbol)) {
                        continue;
                    }

                    vector<string> remaining(production.begin() + i + 1, production.end());
                    set<string> firstRemaining = firstOfSequence(remaining, first);

                    int oldSize = follow[symbol].size();

                    for (const string& value : firstRemaining) {
                        if (value != "eps") {
                            follow[symbol].insert(value);
                        }
                    }

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

ParsingTable buildParsingTable(const Grammar& grammar, const Sets& first, const Sets& follow) {
    ParsingTable table;

    for (const auto& [lhs, productions] : grammar) {
        for (const auto& production : productions) {
            set<string> firstProduction = firstOfSequence(production, first);

            for (const string& terminal : firstProduction) {
                if (terminal != "eps") {
                    table[lhs][terminal] = production;
                }
            }

            if (firstProduction.count("eps")) {
                for (const string& terminal : follow.at(lhs)) {
                    table[lhs][terminal] = production;
                }
            }
        }
    }

    return table;
}

void printSet(const string& name, const Sets& sets) {
    for (const auto& [symbol, values] : sets) {
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

bool parse(const vector<string>& tokens, const ParsingTable& table, const string& startSymbol) {
    vector<string> input = tokens;
    input.push_back("$");

    vector<string> stack = {"$", startSymbol};
    int pointer = 0;

    cout << "\n";
    cout << left << setw(20) << "Stack"
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

        if (top == "$" && current == "$") {
            cout << left << setw(20) << stackString
                 << setw(20) << inputString
                 << "Accept\n";
            return true;
        }

        if (top == current) {
            cout << left << setw(20) << stackString
                 << setw(20) << inputString
                 << "Match " << top << "\n";

            stack.pop_back();
            pointer++;
        }
        else if (table.count(top) && table.at(top).count(current)) {
            vector<string> production = table.at(top).at(current);

            string productionString;
            for (const string& symbol : production) {
                productionString += symbol;
            }

            cout << left << setw(20) << stackString
                 << setw(20) << inputString
                 << top << " -> " << productionString << "\n";

            stack.pop_back();

            if (!(production.size() == 1 && production[0] == "eps")) {
                for (int i = production.size() - 1; i >= 0; i--) {
                    stack.push_back(production[i]);
                }
            }
        }
        else {
            cout << left << setw(20) << stackString
                 << setw(20) << inputString
                 << "Error\n";

            return false;
        }
    }

    return false;
}

int main() {
    Grammar grammar = {
        {"S", {{"a", "A", "B", "e"}}},
        {"A", {{"b", "A", "c"}, {"d"}}},
        {"B", {{"f"}, {"eps"}}}
    };

    string startSymbol = "S";

    Sets first = calculateFirst(grammar);
    Sets follow = calculateFollow(grammar, first, startSymbol);
    ParsingTable table = buildParsingTable(grammar, first, follow);

    cout << "FIRST Sets\n";
    printSet("FIRST", first);

    cout << "\nFOLLOW Sets\n";
    printSet("FOLLOW", follow);

    cout << "\nLL(1) Parsing Table\n";

    for (const auto& [nonTerminal, entries] : table) {
        for (const auto& [terminal, production] : entries) {
            cout << "M[" << nonTerminal << ", " << terminal << "] = "
                 << nonTerminal << " -> ";

            for (const string& symbol : production) {
                cout << symbol;
            }

            cout << "\n";
        }
    }

    vector<string> input = {"a", "d", "f", "e"};

    cout << "\nParsing input: adfe\n";

    parse(input, table, startSymbol);

    return 0;
}

