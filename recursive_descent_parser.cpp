#include<iostream>
#include<vector>
#include<string>

using namespace std;
struct Node
{
    string val;
    Node* left;
    Node* right;
    Node(string v, Node* l = nullptr, Node* r = nullptr) : val(v), left(l), right(r) {}
};
string input;
int ptr_pos = 0;
string tok;

void next_tok()
{
    while(ptr_pos < input.length() && isspace(input[ptr_pos])) ptr_pos++;
    if(ptr_pos >= input.length())
    {
        tok = "";
        return;
    }
    if(input.substr(ptr_pos, 2) == "id")
    {
        tok = "id";
        ptr_pos += 2;
    }
    else if(input.substr(ptr_pos, 3) == "num")
    {
        tok = "num";
        ptr_pos += 3;
    }
    else tok = string(1, input[ptr_pos++]);
}
void match(string expected)
{
    if(tok == expected) next_tok();
}
Node* E();
Node* T();
Node* F();
Node* E_prime(Node*);
Node* T_prime(Node*);

// E -> T E'
Node* E()
{
    return E_prime(T());
}

// T -> F T'
Node* T()
{
    return T_prime(F());
}

// F -> (E) | id | num
Node* F()
{
    Node* n = nullptr;
    if(tok == "id" || tok == "num")
    {
        n = new Node(tok);
        next_tok();
    }
    else if(tok == "(")
    {
        match("(");
        n = E();
        match(")");
    }
    return n;
}

// E' -> + T E' | - T E' | e
Node* E_prime(Node* left)
{
    if(tok == "+" || tok == "-")
    {
        string op = tok;
        next_tok();
        Node* right = T();
        Node* newLeft = new Node(op, left, right);

        return E_prime(newLeft);
    }
    return left;
}

// T' -> * F T' | / F T' | e
Node* T_prime(Node* left)
{
    if(tok == "*" || tok == "/")
    {
        string op = tok;
        next_tok();
        Node* right = F();
        Node* newLeft = new Node(op, left, right);

        return T_prime(newLeft);
    }
    return left;
}

double evaluator(Node* root,double id, double num)
{
    if(root == NULL) return 0;
    if(root->val == "id") return id;
    if(root->val == "num") return num;

    double l = evaluator(root->left, id, num);
    double r = evaluator(root->right, id, num);

    if(root->val == "+") return l + r;
    if(root->val == "-") return l - r;
    if(root->val == "*") return l * r;
    if(root->val == "/") return l / r;
    return 0;
}
void printTree(Node* root,int space = 0)
{
    if(root == NULL) return;
    int spacing = 5;
    space += spacing;
    printTree(root->right, space);
    cout<<endl;
    for(int i=spacing;i<space;i++) cout<<" ";
    cout<<root->val<<endl;
    printTree(root->left, space);
}
int main()
{
    cout<<"Enter the expression: ";
    fflush(stdin);
    getline(cin, input);
    double id, num;
    cout<<"Enter id: ";
    cin>>id;
    cout<<"Enter num: ";
    cin>>num;
    cout<<"=================================================="<<endl;
    ptr_pos = 0;
    next_tok();
    cout<<input<<endl;
    Node* ast = E();
    printTree(ast);
    cout<<input<<" = "<<evaluator(ast, id, num)<<endl;
}