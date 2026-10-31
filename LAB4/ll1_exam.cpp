// ============================================================================
// EXAM-FRIENDLY LL(1) PARSER - Compact version (~90 lines)
// Handles BOTH space-separated AND compact notation (e.g., "aABe")
// Rule: UPPERCASE = non-terminal, lowercase/symbol = terminal
// ============================================================================

#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <cctype>

using namespace std;

using Grammar = map<string, vector<vector<string>>>;
using Sets    = map<string, set<string>>;
using Table   = map<string, map<string, vector<string>>>;

// Tokenize RHS: each char is a symbol, but keep multi-char like "eps"
// Uppercase -> non-terminal, others -> terminal
vector<string> tokenizeRHS(const string& rhs) {
    vector<string> tokens;
    string curr;
    for (size_t i = 0; i < rhs.size(); i++) {
        char c = rhs[i];
        if (c == ' ' || c == '\t') {
            if (!curr.empty()) { tokens.push_back(curr); curr.clear(); }
            continue;
        }
        if (c == '|') {
            if (!curr.empty()) { tokens.push_back(curr); curr.clear(); }
            tokens.push_back("|");
        }
        else if (isupper(c)) {  // Non-terminal
            if (!curr.empty()) { tokens.push_back(curr); curr.clear(); }
            tokens.push_back(string(1, c));
        }
        else {  // Terminal (lowercase, digit, symbol)
            curr += c;
        }
    }
    if (!curr.empty()) tokens.push_back(curr);
    return tokens;
}

// Read grammar from stdin
Grammar readGrammar(string& start) {
    Grammar g;
    string line;
    cout << "Enter grammar (empty line to end):\n";
    cout << "Examples:\n";
    cout << "  S -> aABe        (compact)\n";
    cout << "  A -> bAc | d     (with |)\n";
    cout << "  B -> f | eps\n";
    cout << "  OR space-separated:\n";
    cout << "  S -> a A B e\n";

    bool first = true;
    while (getline(cin, line) && !line.empty()) {
        // Find "->"
        size_t arrow = line.find("->");
        if (arrow == string::npos) continue;

        string lhs = line.substr(0, arrow);
        // Trim lhs
        while (!lhs.empty() && isspace(lhs.back())) lhs.pop_back();
        while (!lhs.empty() && isspace(lhs.front())) lhs.erase(0,1);

        if (first) { start = lhs; first = false; }

        string rhs = line.substr(arrow + 2);
        auto tokens = tokenizeRHS(rhs);

        vector<string> prod;
        for (auto& tok : tokens) {
            if (tok == "|") {
                if (!prod.empty()) { g[lhs].push_back(prod); prod.clear(); }
            } else {
                prod.push_back(tok);
            }
        }
        if (!prod.empty()) g[lhs].push_back(prod);
    }
    return g;
}

// Compute FIRST sets
Sets firstSets(const Grammar& g) {
    Sets first;
    set<string> terms;

    // Find terminals: anything not a non-terminal and not eps
    for (auto& [lhs, prods] : g)
        for (auto& p : prods)
            for (auto& s : p)
                if (!g.count(s) && s != "eps") terms.insert(s);

    for (auto& t : terms) first[t].insert(t);
    first["eps"].insert("eps");

    bool changed = true;
    while (changed) {
        changed = false;
        for (auto& [lhs, prods] : g) {
            for (auto& p : prods) {
                bool nullable = true;
                for (auto& sym : p) {
                    int sz = first[lhs].size();
                    for (auto& v : first[sym]) if (v != "eps") first[lhs].insert(v);
                    if (first[lhs].size() != sz) changed = true;
                    if (!first[sym].count("eps")) { nullable = false; break; }
                }
                if (nullable) {
                    int sz = first[lhs].size();
                    first[lhs].insert("eps");
                    if (first[lhs].size() != sz) changed = true;
                }
            }
        }
    }
    return first;
}

// FIRST of sequence
set<string> firstOf(const vector<string>& seq, const Sets& first) {
    set<string> res; bool all = true;
    for (auto& s : seq) {
        if (s == "eps") { res.insert("eps"); return res; }
        for (auto& v : first.at(s)) if (v != "eps") res.insert(v);
        if (!first.at(s).count("eps")) { all = false; break; }
    }
    if (all) res.insert("eps");
    return res;
}

// Compute FOLLOW sets
Sets followSets(const Grammar& g, const Sets& first, const string& start) {
    Sets follow;
    for (auto& [lhs, _] : g) follow[lhs];
    follow[start].insert("$");

    bool changed = true;
    while (changed) {
        changed = false;
        for (auto& [lhs, prods] : g) {
            for (auto& p : prods) {
                for (int i = 0; i < (int)p.size(); i++) {
                    string sym = p[i];
                    if (!g.count(sym)) continue;
                    vector<string> rem(p.begin() + i + 1, p.end());
                    auto f = firstOf(rem, first);
                    int sz = follow[sym].size();
                    for (auto& v : f) if (v != "eps") follow[sym].insert(v);
                    if (rem.empty() || f.count("eps"))
                        follow[sym].insert(follow[lhs].begin(), follow[lhs].end());
                    if (follow[sym].size() != sz) changed = true;
                }
            }
        }
    }
    return follow;
}

// Build parsing table
Table buildTable(const Grammar& g, const Sets& first, const Sets& follow) {
    Table t;
    for (auto& [lhs, prods] : g) {
        for (auto& p : prods) {
            auto f = firstOf(p, first);
            for (auto& term : f) if (term != "eps") t[lhs][term] = p;
            if (f.count("eps"))
                for (auto& term : follow.at(lhs)) t[lhs][term] = p;
        }
    }
    return t;
}

// Tokenize input string: each char = separate token (except "eps")
// Rule: uppercase = non-terminal (1 char), lowercase/symbol = terminal (1 char)
vector<string> tokenizeInput(const string& s) {
    vector<string> tokens;
    for (size_t i = 0; i < s.size(); i++) {
        char c = s[i];
        if (c == ' ' || c == '\t') continue;
        if (c == 'e' && i+2 < s.size() && s[i+1]=='p' && s[i+2]=='s') {
            tokens.push_back("eps");
            i += 2;
        } else {
            tokens.push_back(string(1, c));
        }
    }
    return tokens;
}

void printSets(const string& name, const Sets& s) {
    cout << "\n" << name << " Sets:\n";
    for (auto& [k, v] : s) {
        if (k == "eps" || !isupper(k[0])) continue;
        cout << "  " << name << "(" << k << ") = { ";
        for (auto& x : v) cout << x << " ";
        cout << "}\n";
    }
}

void printTable(const Table& t) {
    cout << "\nParsing Table:\n";
    for (auto& [nt, row] : t)
        for (auto& [term, prod] : row) {
            cout << "  M[" << nt << "," << term << "] = " << nt << " -> ";
            for (auto& s : prod) cout << s;
            cout << "\n";
        }
}

// Table-driven parse
void parse(const Table& t, const string& start, const vector<string>& input) {
    vector<string> in = input; in.push_back("$");
    vector<string> stk = {"$", start};
    int p = 0;

    cout << "\nParse Trace:\n";
    cout << "Stack\t\tInput\t\tAction\n";
    cout << "------------------------------------------------\n";

    while (!stk.empty()) {
        string top = stk.back(), cur = in[p];
        string ss, is;
        for (auto& s : stk) ss += s;
        for (int i = p; i < (int)in.size(); i++) is += in[i];

        if (top == "$" && cur == "$") { cout << ss << "\t" << is << "\tAccept\n"; return; }
        if (top == cur) { cout << ss << "\t" << is << "\tMatch " << top << "\n"; stk.pop_back(); p++; }
        else if (t.count(top) && t.at(top).count(cur)) {
            auto prod = t.at(top).at(cur);
            string ps; for (auto& s : prod) ps += s;
            cout << ss << "\t" << is << "\t" << top << " -> " << ps << "\n";
            stk.pop_back();
            if (!(prod.size() == 1 && prod[0] == "eps"))
                for (int i = (int)prod.size() - 1; i >= 0; i--) stk.push_back(prod[i]);
        } else { cout << ss << "\t" << is << "\tError\n"; return; }
    }
}

int main() {
    string start;
    Grammar g = readGrammar(start);
    if (g.empty()) return 0;

    auto first = firstSets(g);
    auto follow = followSets(g, first, start);
    auto table = buildTable(g, first, follow);

    printSets("FIRST", first);
    printSets("FOLLOW", follow);
    printTable(table);

    cout << "\nEnter input string (e.g., adfe or a d f e): ";
    string in; getline(cin, in);
    if (in.empty()) return 0;
    parse(table, start, tokenizeInput(in));
}