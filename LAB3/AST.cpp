// ============================================================================
// QUESTION: Write a recursive-descent parser for the following grammar
// and build a basic abstract syntax tree (AST) and evaluate the expressions.
// ============================================================================
//
// GRAMMAR (Left-Recursion-Free):
// -------------------------------
// E  -> T E'
// E' -> + T E' | - T E' | epsilon
// T  -> F T'
// T' -> * F T' | / F T' | epsilon
// F  -> ( E ) | id | num
//
// WHY THIS GRAMMAR?
// -----------------
// Original (left-recursive) grammar would be:
//   E -> E + T | E - T | T
//   T -> T * F | T / F | F
//   F -> ( E ) | id | num
//
// Left recursion (E -> E + T) causes INFINITE RECURSION in recursive descent
// because E() calls E() calls E()... never consuming input.
//
// SOLUTION: Eliminate left recursion using the "E-prime" (E') trick:
//   E  -> T E'        (parse a Term, then handle rest)
//   E' -> + T E' | - T E' | epsilon  (loop for more + or -)
//
// This makes the grammar LL(1) - suitable for recursive descent.
// The same applies to T -> F T' etc.
// ============================================================================
//
// RECURSIVE DESCENT PARSING:
// --------------------------
// Each non-terminal becomes a FUNCTION that returns an AST node.
// The parser reads input LEFT-TO-RIGHT, maintaining a position index.
// Functions call each other according to grammar rules.
// Precedence is encoded by CALL HIERARCHY:
//   E() calls T() calls F()
//   So * and / (in T) bind tighter than + and - (in E)
// ============================================================================
//
// AST (Abstract Syntax Tree):
// ---------------------------
// Each node represents a construct in the source:
//   - Internal nodes: operators (+, -, *, /) with left/right children
//   - Leaf nodes: identifiers (id) and numbers (num)
//
// The tree STRUCTURE encodes precedence:
//        +                           id + id * num
//       / \                                |
//      id   *                              v
//          / \                  AST:       +
//         id  num                        / \
//                                        id   *
//                                            / \
//                                           id  num
//
// During evaluation (post-order traversal), children are evaluated first,
// so * is computed before + automatically!
// ============================================================================

#include <iostream>
#include <string>
#include <cctype>
#include <map>
#include <vector>
using namespace std;


// ============================================================================
// AST NODE STRUCTURE
// ============================================================================
// Simple struct-based node (not OOP). Each node has:
//   - type: "OP" for operators, "ID" for identifiers, "NUM" for numbers
//   - value: the operator symbol ("+", "-", "*", "/") or token name ("id", "num")
//   - left, right: children (null for leaves)
// ============================================================================
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


// ============================================================================
// GLOBAL PARSER STATE
// ============================================================================
string input;              // The expression being parsed
int pos = 0;               // Current position in input string
map<string, int> values = { // Environment for evaluation
    {"id", 10},            // id = 10 (hardcoded for demo)
    {"num", 5}             // num = 5 (hardcoded for demo)
};


// ============================================================================
// skipSpaces(): Utility to skip whitespace
// ============================================================================
// WHY? The grammar doesn't have explicit whitespace tokens.
// We must skip spaces before EVERY token check.
// Called at the start of match() and F().
// ============================================================================
void skipSpaces() {
    while (pos < input.length() && isspace(input[pos]))
        pos++;
}


// ============================================================================
// match(char c): Core parsing primitive
// ============================================================================
// Attempts to match character 'c' at current position.
// 1. Skips whitespace
// 2. If current char == c: advances pos and returns true
// 3. Otherwise: returns false (no advance)
//
// This is the "lookahead + consume" pattern used throughout the parser.
// ============================================================================
bool match(char c) {
    skipSpaces();

    if (pos < input.length() && input[pos] == c) {
        pos++;
        return true;
    }

    return false;
}


// Forward declarations (mutual recursion: E -> T -> F -> E)
Node* E();
Node* T();
Node* F();


// ============================================================================
// E(): Parse Expression -> handles + and - (LOWEST precedence)
// ============================================================================
// Grammar: E -> T E'
// Implemented as: parse T, then loop matching + or - followed by T
//
// WHY while(true) with break?
// Because E' is right-recursive: E' -> + T E' | epsilon
// This loop implements the "zero or more (+/- T)" pattern iteratively.
// Each iteration builds a new OP node with left = previous result,
// right = new T(). This creates LEFT-ASSOCIATIVE trees:
//   id + id + id  =>  (id + id) + id
// ============================================================================
Node* E() {
    Node* left = T();          // Parse first term (handles * and /)

    while (true) {
        skipSpaces();

        if (match('+')) {
            Node* right = T();
            left = new Node("OP", "+", left, right);  // Build (+ left right)
        }
        else if (match('-')) {
            Node* right = T();
            left = new Node("OP", "-", left, right);  // Build (- left right)
        }
        else {
            break;  // No more + or - -> epsilon production
        }
    }

    return left;
}


// ============================================================================
// T(): Parse Term -> handles * and / (HIGHER precedence than +,-)
// ============================================================================
// Grammar: T -> F T'
// Implemented as: parse F, then loop matching * or / followed by F
//
// SAME PATTERN as E() but for multiplication/division.
// Because T() is called by E(), * and / bind tighter:
//   id + id * num
//   E() calls T() for first "id"
//   Then sees +, calls T() again
//   Second T() calls F() for "id", then sees *, calls F() for "num"
//   So * is LOWER in the tree (evaluated first)
// ============================================================================
Node* T() {
    Node* left = F();          // Parse first factor

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
            break;  // No more * or / -> epsilon
        }
    }

    return left;
}


// ============================================================================
// F(): Parse Factor -> handles atoms: (E), id, num (HIGHEST precedence)
// ============================================================================
// Grammar: F -> ( E ) | id | num
//
// This is the BASE CASE of the recursion. It either:
// 1. Sees '(' -> parses E() -> expects ')'
// 2. Sees "id" -> returns ID leaf node
// 3. Sees "num" -> returns NUM leaf node
//
// WHY check "id" before "num"? Because "id" starts with 'i' and "num" with 'n'.
// They're distinct. But we check substrings explicitly since we don't
// have a separate lexer - we parse directly from the string.
// ============================================================================
Node* F() {
    skipSpaces();
    if (match('(')) {
        Node* node = E();      // Parse inner expression

        if (!match(')')) {
            cout << "Error: Missing ')'\n";
            exit(1);
        }

        return node;           // Parentheses just group, no node created
    }

    // Check for "id" token (2 chars)
    if (input.substr(pos, 2) == "id") {
        pos += 2;
        return new Node("ID", "id");
    }

    // Check for "num" token (3 chars)
    if (input.substr(pos, 3) == "num") {
        pos += 3;
        return new Node("NUM", "num");
    }

    cout << "Error: Invalid token at position " << pos << "\n";
    exit(1);
}


// ============================================================================
// evaluate(Node* root): Post-order tree traversal to compute value
// ============================================================================
// Recursively evaluates the AST:
//   - If leaf (ID or NUM): return value from environment
//   - If operator: evaluate left and right, then apply operator
//
// This is a POST-ORDER traversal: children evaluated before parent.
// This naturally respects operator precedence encoded in tree structure.
// ============================================================================
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
        return left / right;   // Integer division

    return 0;
}


// ============================================================================
// SIMPLE TREE PRINTING - Top-down with / and \ branches
// ============================================================================
// Returns vector of lines for this subtree, recursively.
// For binary expression trees: each internal node has 2 children (or 0).
// ============================================================================
vector<string> treeLines(Node* n) {
    if (!n) return {};
    string label = n->value;
    auto L = treeLines(n->left);
    auto R = treeLines(n->right);

    // Leaf: just the label
    if (L.empty() && R.empty()) return {label};

    // Width of left/right blocks (first line width)
    int lw = L.empty() ? 0 : (int)L[0].size();
    int rw = R.empty() ? 0 : (int)R[0].size();

    // Root centered over children with 1 space gap
    int rootPos = lw + 1;
    int totalW  = lw + 1 + rw + 1;

    vector<string> out;

    // Line 0: root label centered over left block
    string top(totalW, ' ');
    for (int i = 0; i < (int)label.size(); i++)
        top[rootPos - (int)label.size()/2 + i] = label[i];
    out.push_back(top);

    // Line 1: branches \ /
    string br(totalW, ' ');
    if (!L.empty()) br[rootPos - 1] = '/';
    if (!R.empty()) br[rootPos + 1] = '\\';
    out.push_back(br);

    // Remaining lines: merge L and R side by side
    int rows = max((int)L.size(), (int)R.size());
    for (int i = 0; i < rows; i++) {
        string row(totalW, ' ');
        if (i < (int)L.size()) row.replace(0, lw, L[i]);
        if (i < (int)R.size()) row.replace(lw + 2, rw, R[i]);
        out.push_back(row);
    }
    return out;
}

void printAST(Node* root) {
    if (!root) return;
    for (const string& line : treeLines(root))
        cout << line << "\n";
}


// ============================================================================
// parseAndEvaluate(): Top-level function for one expression
// ============================================================================
void parseAndEvaluate(string expr) {
    input = expr;
    pos = 0;

    Node* root = E();          // Start parsing from start symbol E

    skipSpaces();

    // After parsing, should have consumed ALL input
    if (pos != input.length()) {
        cout << "Invalid expression (extra characters)\n";
        return;
    }

    cout << "Expression: " << expr << '\n';
    cout << "AST:\n";
    printAST(root);
    cout << "Value: " << evaluate(root) << "\n\n";
}


// ============================================================================
// MAIN: Test with the two required expressions
// ============================================================================
int main() {

    cout << "Given Grammar :" << endl;
    cout << "E -> TE'\nE' -> +TE' | -TE' | e\nT -> FT' \nT' -> *FT' | /FT' | e \nF -> (E) | id | num\n" << endl << endl;

    parseAndEvaluate("id + id * num");
    parseAndEvaluate("(id - num) / id");

    return 0;
}


// ============================================================================
// KEY CONCEPTS FOR EXAM:
// ============================================================================
//
// 1. LEFT RECURSION ELIMINATION:
//    Original: E -> E + T | T          (breaks recursive descent)
//    Fixed:    E -> T E',  E' -> + T E' | epsilon
//
// 2. PRECEDENCE VIA CALL HIERARCHY:
//    E() calls T() calls F()
//    Lower in call stack = higher precedence (evaluated first)
//
// 3. LEFT ASSOCIATIVITY:
//    The while-loop in E() and T() builds left-associative trees:
//    left = new Node(op, left, right)  -- previous result becomes left child
//
// 4. MATCH FUNCTION PATTERN:
//    skipSpaces() -> check char -> if match: advance and return true
//    Always call skipSpaces() BEFORE checking!
//
// 5. BASE CASE (F):
//    Handles parentheses (recursive call to E), identifiers, numbers
//    This is where the recursion BOTTOMS OUT
//
// 6. AST EVALUATION:
//    Post-order traversal: left, right, then operator
//    Tree structure enforces precedence automatically
//
// 7. COMMON EXAM VARIATIONS:
//    - Add more operators: %, ^, unary -, etc.
//    - Add variables: let x = 5 in x + 3
//    - Build actual symbol table instead of hardcoded map
//    - Add error recovery (not just exit(1))
//
// ============================================================================