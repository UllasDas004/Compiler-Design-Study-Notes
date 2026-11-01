#include<iostream>
#include<vector>
#include<string>
#include<set>
#include<map>
#include<iomanip>

using namespace std;

struct Rule
{
    string lhs;
    vector<string> rhs;
};

struct Item
{
    int rule_idx;
    int dot_pos;
    bool operator<(const Item& o) const
    {
        return rule_idx != o.rule_idx ? rule_idx < o.rule_idx : dot_pos < o.dot_pos;
    }
    bool operator==(const Item& o) const
    {
        return rule_idx == o.rule_idx && dot_pos == o.dot_pos;
    }
};

using State = set<Item>;
vector<Rule> rules;

State closure(State I)
{
    State J = I;
    bool changed = true;
    while(changed)
    {
        changed = false;
        State temp = J;
        for(auto item : temp)
        {
            if(item.dot_pos < rules[item.rule_idx].rhs.size())
            {
                string B = rules[item.rule_idx].rhs[item.dot_pos];
                for(int i=0;i<(int)rules.size();i++)
                {
                    if(rules[i].lhs == B)
                    {
                        Item new_item = {i, 0};
                        if(!J.count(new_item))
                        {
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

State GOTO(State I, const string& X)
{
    State J;
    for(auto item : I)
    {
        if(item.dot_pos < rules[item.rule_idx].rhs.size() && rules[item.rule_idx].rhs[item.dot_pos] == X)
        J.insert({item.rule_idx, item.dot_pos + 1});
    }
    return closure(J);
}

vector<State> states;
map<pair<int, string>, int> trans;

void buildStates()
{
    states.clear();
    trans.clear();
    states.push_back(closure({{0,0}}));

    bool changed = true;
    while(changed)
    {
        changed = false;
        for(size_t i=0;i<states.size();i++)
        {
            set<string> syms;
            for(auto item : states[i])
            {
                if(item.dot_pos < rules[item.rule_idx].rhs.size())
                syms.insert(rules[item.rule_idx].rhs[item.dot_pos]);
            }
            for(auto X : syms)
            {
                State nxt = GOTO(states[i], X);
                if(nxt.empty()) continue;
                int idx = -1;
                for(size_t j=0;j<states.size();j++)
                {
                    if(states[j] == nxt)
                    {
                        idx = j;
                        break;
                    }
                }
                if(idx == -1)
                {
                    states.push_back(nxt);
                    idx = states.size() - 1;
                    changed = true;
                }
                trans[{(int)i, X}] = idx;
            }
        }
    }
}

map<int, map<string, string>> action;
map<int, map<string, int>> go_to;

void buildTables()
{
    action.clear();
    go_to.clear();
    set<string> terms, nts;
    for(auto& r : rules) nts.insert(r.lhs);
    for(auto& r : rules)
    {
        for(auto& s : r.rhs)
        {
            if(!nts.count(s) && s != "eps")
            terms.insert(s);
            terms.insert("$");
        }
    }
    terms.insert("$");
    for(size_t i=0;i<states.size();i++)
    {
        for(auto item : states[i])
        {
            if(item.dot_pos < rules[item.rule_idx].rhs.size())
            {
                string a = rules[item.rule_idx].rhs[item.dot_pos];
                if(terms.count(a)) action[i][a] = "s" + to_string(trans[{(int)i, a}]);
            }
        }
        for(auto item : states[i])
        {
            if(item.dot_pos == rules[item.rule_idx].rhs.size())
            {
                if(item.rule_idx == 0) action[i]["$"] = "Acc";
                else
                {
                    for(auto t : terms)
                    {
                        if(action[i][t].empty()) action[i][t] = "r" + to_string(item.rule_idx);
                        else action[i][t] += "/r" + to_string(item.rule_idx);
                    }
                }
            }
        }
        for(auto A : nts)
        {
            if(trans.count({(int)i, A}))
            go_to[i][A] = trans[{(int)i, A}];
        }
    }
}

void parse(const string& input_str)
{
    vector<string> input;
    for(char c : input_str) if(c != ' ') input.push_back(string(1,c));
    input.push_back("$");
    vector<int> st = {0};
    vector<string> sym;
    int ptr = 0;
    cout << "\nParsing: " << input_str << "\n";
    cout << left << setw(18) << "State Stack" << " | "
         << setw(18) << "Symbol Stack" << " | "
         << setw(10) << "Input" << " | Action\n";
    cout << string(60, '-') << "\n";
    while(true)
    {
        int s = st.back();
        string a = input[ptr];
        string ss, sy, is;
        for(int x : st) ss += to_string(x) + " ";
        for(auto x : sym) sy += x + " ";
        for(size_t i=ptr;i<input.size();i++) is += input[i];
        string act = action[s][a];
        if(act.empty())
        {
            cout << left << setw(18) << ss << " | " << setw(18) << sy << " | " << setw(10) << is << " | Error\n";
            break;
        }
        string act0 = act.find('/') != string::npos ? act.substr(0, act.find('/')) : act;
        string action_str;
        if(act0[0] == 's')
        {
            action_str = "Shift " + act0.substr(1);
        }
        else if(act0[0] == 'r')
        {
            int ri = stoi(act0.substr(1));
            Rule r = rules[ri];
            action_str = "Reduce " + to_string(ri) + " (" + r.lhs + " -> ";
            for (auto x : r.rhs) action_str += x + " ";
            action_str += ")";
        }
        else if(act0 == "Acc")
        {
            action_str = "Accept";
        }
        cout << left << setw(18) << ss << " | " << setw(18) << sy << " | " << setw(10) << is << " | " << action_str << "\n";

        if(act0[0] == 's')
        {
            st.push_back(stoi(act0.substr(1)));
            sym.push_back(a);
            ptr++;
        }
        else if(act0[0] == 'r')
        {
            int ri = stoi(act0.substr(1));
            Rule r = rules[ri];
            for(size_t i=0;i<r.rhs.size();i++)
            {
                st.pop_back();
                sym.pop_back();
            }
            int top = st.back();
            sym.push_back(r.lhs);
            st.push_back(go_to[top][r.lhs]);
        }
        else if(act0 == "Acc")
        {
            break;
        }
    }
}

int main()
{
    rules = 
    {
        {"S'", {"S"}},
        {"S",  {"A"}},
        {"A",  {"A", "b"}},
        {"A",  {"a"}}
    };
    buildStates();
    buildTables();
    
    // Print table
    set<string> terms, nts;
    for (auto& r : rules) nts.insert(r.lhs);           // Pass 1: all NTs
    for (auto& r : rules)                               // Pass 2: terminals from RHS
        for (auto& s : r.rhs)
            if (!nts.count(s) && s != "eps") terms.insert(s);
    terms.insert("$");
    
    cout << "\nACTION Table:\nState\t";
    for (auto t : terms) cout << t << "\t";
    cout << "\n";
    for (size_t i = 0; i < states.size(); i++) {
        cout << i << "\t";
        for (auto t : terms) cout << (action[i].count(t) ? action[i][t] : "") << "\t";
        cout << "\n";
    }
    
    cout << "\nGOTO Table:\nState\t";
    for (auto A : nts) if (A != "S'") cout << A << "\t";
    cout << "\n";
    for (size_t i = 0; i < states.size(); i++) {
        cout << i << "\t";
        for (auto A : nts) if (A != "S'") cout << (go_to[i].count(A) ? to_string(go_to[i][A]) : "") << "\t";
        cout << "\n";
    }

    parse("abb");

    return 0;
}