#include<iostream>
#include<string>
#include<vector>
#include<set>
#include<map>
#include<iomanip>
using namespace std;

struct Rule
{
    char lhs;
    string rhs;
};

vector<Rule> rules;
map<char, set<char>> first;

bool is_terminal(char c)
{
    return islower(c) || c == '$';
}

set<char> first_of_sym(char X)
{
    if(is_terminal(X)) return {X};
    if(X == '#') return {'#'};

    if(first.count(X)) return first[X];

    set<char> result;
    for(auto& r : rules)
    {
        if(r.lhs != X) continue;
        if(r.rhs.empty()) result.insert('#');
        else
        {
            bool all_nullable = true;
            for(char Y : r.rhs)
            {
                set<char> fy = first_of_sym(Y);
                for(char c : fy) if(c != '#') result.insert(c);
                if(!fy.count('#'))
                {
                    all_nullable = false;
                    break;
                }
            }
            if(all_nullable) result.insert('#');
        }
    }
    first[X] = result;
    return result;
}

void compute_first()
{
    for(auto& r : rules) first_of_sym(r.lhs);
}

void print_first()
{
    cout << "\nFIRST sets:\n";
    for(auto& [nt, fs] : first)
    {
        cout << "FIRST(" << nt << ") = { ";
        for(char c : fs) cout<<c<<" ";
        cout<<"}\n";
    }
}

map<char, set<char>> follow;

set<char> first_of_seq(const string& seq)
{
    set<char> result;
    bool all_nullable = true;
    for(char Y : seq)
    {
        set<char> fy = first_of_sym(Y);
        for(char c : fy) if(c != '#') result.insert(c);
        if(!fy.count('#'))
        {
            all_nullable = false;
            break;
        }
    }
    if(all_nullable) result.insert('#');
    return result;
}

void compute_follow()
{
    follow[rules[0].lhs].insert('$');

    bool changed = true;
    while(changed)
    {
        changed = false;
        for(auto& r : rules)
        {
            char A = r.lhs;
            const string& rhs = r.rhs;
            for(int i=0;i<(int)rhs.size();i++)
            {
                char B = rhs[i];
                if(!isupper(B)) continue;

                string beta = rhs.substr(i+1);
                set<char> first_beta = first_of_seq(beta);
                for(char c : first_beta)
                {
                    if(c != '#' && follow[B].insert(c).second)
                    changed = true;
                }
                if(beta.empty() || first_beta.count('#'))
                {
                    for(char c : follow[A])
                    {
                        if(follow[B].insert(c).second)
                        changed = true;
                    }
                }
            }
        }
    }
}

void print_follow()
{
    cout << "\nFOLLOW sets:\n";
    for (auto& [nt, fs] : follow)
    {
        cout << "FOLLOW(" << nt << ") = { ";
        for (char c : fs) cout << c << " ";
        cout << "}\n";
    }
}

map<char, map<char, string>> table;

void build_table()
{
    for(auto& r : rules)
    {
        char A = r.lhs;
        const string& alpha = r.rhs;
        set<char> first_alpha = first_of_seq(alpha);

        for(char a : first_alpha)
        {
            if(a != '#')
            table[A][a] = alpha;
        }
        if(first_alpha.count('#'))
        {
            for(char b : follow[A])
            table[A][b] = alpha;
        }
    }
}

void print_table() {
    cout << "\nLL(1) Parsing Table:\n";
    // Collect all terminals
    set<char> terms;
    for (auto& [A, row] : table)
        for (auto& [a, prod] : row)
            terms.insert(a);
    
    // Print header
    cout << setw(6) << "";
    for (char t : terms) cout << setw(6) << t;
    cout << "\n";
    
    // Print rows
    for (auto& [A, row] : table) {
        cout << setw(6) << A;
        for (char t : terms) {
            string prod = row.count(t) ? row[t] : "";
            cout << setw(6) << (prod.empty() ? "-" : prod);
        }
        cout << "\n";
    }
}

void parse_input(const string& input_str)
{
    cout << "\nParsing input: " << input_str << "\n";
    vector<char> input;
    for(char c : input_str)
    {
        if(c != ' ')
        input.push_back(c);
    }
    input.push_back('$');
    vector<char> stack = {'$', 'S'};
    size_t ptr = 0;
    cout << left << setw(25) << "Stack" << setw(20) << "Input" << "Action\n";
    cout << string(70, '-') << "\n";
    while(!stack.empty())
    {
        char top = stack.back();
        char cur = input[ptr];

        string stack_str;
        for(auto it = stack.rbegin();it != stack.rend();it++)
        stack_str += *it;
        string input_str;
        for(size_t i = ptr;i<input.size();i++) input_str += input[i];
        if(top == cur)
        {
            if(top == '$')
            {
                cout << left << setw(25) << stack_str << setw(20) << input_str << "Accept\n";
                break;
            }
            cout << left << setw(25) << stack_str << setw(20) << input_str << "Match " << top << "\n";
            stack.pop_back();
            ptr++;
        }
        else if(isupper(top))
        {
            if(table[top].count(cur) && !table[top][cur].empty())
            {
                string prod = table[top][cur];
                cout << left << setw(25) << stack_str << setw(20) << input_str 
                     << top << " -> " << prod << "\n";
                stack.pop_back();
                for(int i=prod.size()-1;i>=0;i--)
                {
                    if(prod[i] != '#')
                    stack.push_back(prod[i]);
                }
            }
            else
            {
                cout << left << setw(25) << stack_str << setw(20) << input_str << "Error: no entry\n";
                break;
            }
        }
        else
        {  // Terminal mismatch
            cout << left << setw(25) << stack_str << setw(20) << input_str << "Error: mismatch\n";
            break;
        }
    }
}

int main()
{
    // Grammar:
    // S -> a A B
    // A -> b A c | d
    // B -> f | eps
    rules = {
        {'S', "aAB"},
        {'A', "bAc"}, {'A', "d"},
        {'B', "f"}, {'B', ""} // epsilon = empty string
    };
    compute_first();
    print_first();
    compute_follow();
    print_follow();
    build_table();
    print_table();
    parse_input("adf");
    parse_input("abcccf");
    return 0;
}