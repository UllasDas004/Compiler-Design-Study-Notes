// Write a recursive-descent parser
// for the following grammar and
// build a basic abstract syntax tree (AST)
// and evaluate the expression id + id * num, (id - num) / id.

// Grammar:
// E  → T E'
// E' → + T E' | - T E' | ε
// T  → F T'
// T' → * F T' | / F T' | ε
// F  → ( E ) | id | num

#include <iostream>
#include <string>
#include <cctype>
#include <map>
using namespace std;

struct Node {
    string type;
    string value;
    Node* left;
    Node* right;

    Node(string t, string v = "", Node* l = nullptr, Node* r = nullptr) {
        type = t;
        value = v;
        left = l;
        right = r;
    }
};

string input;
int pos = 0;
map<string, int> values = {
    {"id", 10},
    {"num", 5}
};

void skipSpaces() {
    while (pos < input.length() && isspace(input[pos]))
        pos++;
}

bool match(char c) {
    skipSpaces();

    if (pos < input.length() && input[pos] == c) {
        pos++;
        return true;
    }

    return false;
}

Node* E();
Node* T();
Node* F();

Node* E() {
    Node* left = T();

    while (true) {
        skipSpaces();

        if (match('+')) {
            Node* right = T();
            left = new Node("OP", "+", left, right);
        }
        else if (match('-')) {
            Node* right = T();
            left = new Node("OP", "-", left, right);
        }
        else {
            break;
        }
    }

    return left;
}

Node* T() {
    Node* left = F();

    while (true) {
        skipSpaces();

        if (match('*')) {
            Node* right = F();
            left = new Node("OP", "*", left, right);
        }
        else if (match('/')) {
            Node* right = F();
            left = new Node("OP", "/", left, right);
        }
        else {
            break;
        }
    }

    return left;
}

Node* F() {
    skipSpaces();
    if (match('(')) {
        Node* node = E();

        if (!match(')')) {
            cout << "Error: Missing ')'\n";
            exit(1);
        }

        return node;
    }

    if (input.substr(pos, 2) == "id") {
        pos += 2;
        return new Node("ID", "id");
    }

    if (input.substr(pos, 3) == "num") {
        pos += 3;
        return new Node("NUM", "num");
    }

    cout << "Error: Invalid token\n";
    exit(1);
}

int evaluate(Node* root) {
    if (root->type == "ID")
        return values["id"];

    if (root->type == "NUM")
        return values["num"];

    int left = evaluate(root->left);
    int right = evaluate(root->right);

    if (root->value == "+")
        return left + right;

    if (root->value == "-")
        return left - right;

    if (root->value == "*")
        return left * right;

    if (root->value == "/")
        return left / right;

    return 0;
}

void printAST(Node* root, int level = 0) {
    if (!root)
        return;

    for (int i = 0; i < level; i++)
        cout << "  ";

    if (root->type == "OP")
        cout << root->value << '\n';
    else
        cout << root->value << '\n';

    printAST(root->left, level + 1);
    printAST(root->right, level + 1);
}

void parseAndEvaluate(string expr) {
    input = expr;
    pos = 0;

    Node* root = E();

    skipSpaces();

    if (pos != input.length()) {
        cout << "Invalid expression\n";
        return;
    }

    cout << "Expression: " << expr << '\n';
    cout << "AST:\n";
    printAST(root);
    cout << "Value: " << evaluate(root) << "\n\n";
}

int main() {

	cout<<"Given Grammer :" <<endl;
	cout<<"E -> TE'\nE' -> +TE' | -TE' | e\nT -> FT' \nT' -> *FT' | /FT' | e \nF -> (E) | id | num\n"<<endl<<endl;
    
    parseAndEvaluate("id + id * num");
    parseAndEvaluate("(id - num) / id");

    return 0;
}
