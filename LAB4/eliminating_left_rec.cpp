#include <iostream>
#include <vector>
#include <string>
#include<set>
using namespace std;
struct Prod
{
    string lhs;
    string rhs;
};

vector<Prod> grammar, result;

void partition(const string &A, vector<string> &alphas, vector<string> &betas)
{
    alphas.clear();
    betas.clear();
    for (auto &p : grammar)
    {
        if (p.lhs != A)
            continue;
        size_t pos = 0;
        while (pos < p.rhs.size() && p.rhs[pos] == ' ')
            pos++;
        if (pos < p.rhs.size() && p.rhs.substr(pos, A.size()) == A)
        {
            pos += A.size();
            while (pos < p.rhs.size() && p.rhs[pos] == ' ')
                pos++;
            alphas.push_back(p.rhs.substr(pos));
        }
        else
            betas.push_back(p.rhs);
    }
}

void eliminate(const string &A)
{
    vector<string> alphas, betas;
    partition(A, alphas, betas);
    if(alphas.empty())
    {
        for(auto& p : grammar)
        {
            if(p.lhs == A)
            result.push_back(p);
        }
        return;
    }
    string Aprime = A + "'";

    for(auto& beta: betas) result.push_back({A, beta + " " + Aprime});
    for(auto& alpha : alphas) result.push_back({Aprime, alpha + " " + Aprime});
    result.push_back({Aprime, "eps"});
}

void eliminateLeftRecursion()
{
    result.clear();
    set<string> nonTerminals;
    for(auto& p : grammar)
    {
        if(!nonTerminals.count(p.lhs))
        nonTerminals.insert(p.lhs);
    }
    for(auto& nt : nonTerminals) eliminate(nt);
}

void printGrammar(const vector<Prod> &g, const string &title)
{
    cout << title << ":\n";
    for (size_t i = 0; i < g.size(); i++)
    {
        if (i == 0 || g[i].lhs != g[i - 1].lhs)
        {
            if (i > 0)
                cout << "\n";
            cout << g[i].lhs << " -> ";
        }
        else
        {
            cout << "     | ";
        }
        cout << g[i].rhs << "\n";
    }
    cout << "\n";
}

int main()
{
    // Input: E -> E + T | T,  T -> T * F | F,  F -> ( E ) | id
    grammar.push_back({"E", "E + T"});
    grammar.push_back({"E", "T"});
    grammar.push_back({"T", "T * F"});
    grammar.push_back({"T", "F"});
    grammar.push_back({"F", "( E )"});
    grammar.push_back({"F", "id"});

    printGrammar(grammar, "Original Grammar");
    eliminateLeftRecursion();
    printGrammar(result, "After Eliminating Left Recursion");
    return 0;
}
