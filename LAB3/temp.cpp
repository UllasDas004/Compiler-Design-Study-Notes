// ============================================================================
// RECURSIVE DESCENT PARSER - OOP VERSION (Clean Architecture)
// ============================================================================
//
// SAME GRAMMAR AS que1.cpp:
// E  -> T E'
// E' -> + T E' | - T E' | epsilon
// T  -> F T'
// T' -> * F T' | / F T' | epsilon
// F  -> ( E ) | id | num
//
// KEY DIFFERENCES FROM que1.cpp:
// ------------------------------
// 1. SEPARATE LEXER CLASS: Tokenizes input into Token stream first
// 2. POLYMORPHIC AST: Abstract base class + derived classes per node type
// 3. PROPER MEMORY MANAGEMENT: Virtual destructors, RAII
// 4. EXCEPTION HANDLING: try/catch instead of exit(1)
// 5. ENVIRONMENT PASSED AS PARAMETER: Not global map
// 6. TOKEN-BASED PARSING: Parser works on Token stream, not raw string
// ============================================================================
//
// WHY THIS DESIGN IS BETTER:
// --------------------------
// - Separation of concerns: Lexer knows chars, Parser knows grammar
// - Extensible: Easy to add new node types (just new class)
// - Testable: Can test lexer and parser independently
// - Real-world pattern: How production compilers are structured
// ============================================================================

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cctype>
#include <stdexcept>

using namespace std;


// ============================================================================
// TOKEN SYSTEM
// ============================================================================
// Tokens are the output of the Lexer, input to the Parser.
// Each token has a TYPE (enum) and optional VALUE (string).
// ============================================================================
enum TokenType { ID, NUM, PLUS, MINUS, MUL, DIV, LPAREN, RPAREN, END };

struct Token {
    TokenType type;
    string value;
};


// ============================================================================
// LEXER CLASS: Converts input string to token stream
// ============================================================================
// Responsibilities:
//  - Skip whitespace
//  - Recognize identifiers, numbers, operators, parentheses
//  - Return tokens one at a time via getNextToken()
//
// This is a HAND-WRITTEN lexer (not generated like Flex).
// In production, you'd use Flex/Lex to generate this from regex rules.
// ============================================================================
class Lexer {
    string text;
    size_t pos;              // Current position in input

public:
    Lexer(const string& input) : text(input), pos(0) {}

    Token getNextToken() {
        // 1. Skip whitespace
        while (pos < text.length() && isspace(text[pos]))
            pos++;

        // 2. End of input
        if (pos >= text.length())
            return {END, ""};

        char current = text[pos];

        // 3. IDENTIFIER or KEYWORD (starts with letter)
        // Note: In this simplified version, "num" is treated as a special
        // identifier that the evaluator maps to a value. A full version
        // would have separate token types for keywords.
        if (isalpha(current)) {
            string id = "";
            while (pos < text.length() && isalnum(text[pos])) {
                id += text[pos++];
            }

            // Special handling: "num" is a token, not an identifier
            // In a real compiler, you'd have a keyword table here
            if (id == "num")
                return {NUM, id};
            return {ID, id};
        }

        // 4. NUMBER (starts with digit)
        if (isdigit(current)) {
            string num = "";
            while (pos < text.length() && isdigit(text[pos])) {
                num += text[pos++];
            }
            return {NUM, num};
        }

        // 5. SINGLE-CHARACTER TOKENS
        pos++;  // Consume the character
        switch (current) {
            case '+': return {PLUS, "+"};
            case '-': return {MINUS, "-"};
            case '*': return {MUL, "*"};
            case '/': return {DIV, "/"};
            case '(': return {LPAREN, "("};
            case ')': return {RPAREN, ")"};
            default:
                throw runtime_error(string("Unknown character: ") + current);
        }
    }
};


// ============================================================================
// ABSTRACT SYNTAX TREE - POLYMORPHIC DESIGN
// ============================================================================
// Base class defines interface: evaluate() and print()
// Each concrete node type implements these.
// This is the VISITOR pattern (without explicit visitor) -
// operations are methods on the nodes themselves.
// ============================================================================

class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual int evaluate(const unordered_map<string, int>& env) = 0;
    virtual void print(int indent = 0) = 0;
};


// ---- LEAF NODES ----

// IDENTIFIER NODE: Variable reference (e.g., "id", "x", "count")
class IdNode : public ASTNode {
    string name;
public:
    IdNode(const string& n) : name(n) {}
    int evaluate(const unordered_map<string, int>& env) override {
        return env.at(name);    // Look up value in environment
    }
    void print(int indent) override {
        cout << string(indent, ' ') << "ID(" << name << ")" << endl;
    }
};

// NUMBER NODE: Numeric literal (e.g., "5", "42") or special "num"
class NumNode : public ASTNode {
    string value;
public:
    NumNode(const string& v) : value(v) {}
    int evaluate(const unordered_map<string, int>& env) override {
        // Special: if value is "num", look up in env
        // Otherwise parse as integer
        if (value == "num")
            return env.at("num");
        return stoi(value);
    }
    void print(int indent) override {
        cout << string(indent, ' ') << "NUM(" << value << ")" << endl;
    }
};


// ---- INTERNAL NODES ----

// BINARY OPERATOR NODE: +, -, *, / with left and right children
class BinaryOpNode : public ASTNode {
    char op;
    ASTNode* left;
    ASTNode* right;
public:
    BinaryOpNode(char o, ASTNode* l, ASTNode* r) : op(o), left(l), right(r) {}

    // RAII: Destructor deletes children (owns them)
    ~BinaryOpNode() { delete left; delete right; }

    int evaluate(const unordered_map<string, int>& env) override {
        int l_val = left->evaluate(env);
        int r_val = right->evaluate(env);
        switch (op) {
            case '+': return l_val + r_val;
            case '-': return l_val - r_val;
            case '*': return l_val * r_val;
            case '/': return l_val / r_val;
            default: return 0;
        }
    }
    void print(int indent) override {
        cout << string(indent, ' ') << "Op: " << op << endl;
        left->print(indent + 2);
        right->print(indent + 2);
    }
};


// ============================================================================
// PARSER CLASS: Recursive descent parser on token stream
// ============================================================================
// Uses LOOKAHEAD TOKEN (currentToken) to make parsing decisions.
// advance() fetches next token from lexer.
// match(type) checks if current token matches and advances if so.
// expect(type) requires a match or throws error.
// ============================================================================
class Parser {
    Lexer lexer;
    Token currentToken;      // The lookahead token (1-token peek)

    // Fetch next token from lexer
    void advance() {
        currentToken = lexer.getNextToken();
    }

    // Try to match and consume token of given type
    // Returns true if matched, false otherwise
    bool match(TokenType type) {
        if (currentToken.type == type) {
            advance();
            return true;
        }
        return false;
    }

    // Require a token of given type, throw if not found
    void expect(TokenType type) {
        if (!match(type)) {
            throw runtime_error("Syntax error: Unexpected token");
        }
    }

public:
    Parser(const string& text) : lexer(text) {
        advance();           // Prime the lookahead with first token
    }

    // Entry point: parse full expression
    ASTNode* parse() {
        ASTNode* ast = parseE();
        if (currentToken.type != END) {
            throw runtime_error("Syntax error: Unexpected characters at end");
        }
        return ast;
    }

    // ========================================================================
    // GRAMMAR RULES AS METHODS:
    // Each non-terminal = one method
    // ========================================================================

    // E -> T E'
    ASTNode* parseE() {
        ASTNode* left = parseT();
        return parseEPrime(left);
    }

    // E' -> + T E' | - T E' | epsilon
    // Implemented as RECURSIVE function (not loop like que1.cpp)
    // This directly mirrors the right-recursive grammar!
    // parseEPrime(left) returns the combined tree
    ASTNode* parseEPrime(ASTNode* left) {
        if (currentToken.type == PLUS) {
            advance();
            ASTNode* right = parseT();
            return parseEPrime(new BinaryOpNode('+', left, right));
        } else if (currentToken.type == MINUS) {
            advance();
            ASTNode* right = parseT();
            return parseEPrime(new BinaryOpNode('-', left, right));
        }
        return left;  // epsilon: no more + or -
    }

    // T -> F T'
    ASTNode* parseT() {
        ASTNode* left = parseF();
        return parseTPrime(left);
    }

    // T' -> * F T' | / F T' | epsilon
    ASTNode* parseTPrime(ASTNode* left) {
        if (currentToken.type == MUL) {
            advance();
            ASTNode* right = parseF();
            return parseTPrime(new BinaryOpNode('*', left, right));
        } else if (currentToken.type == DIV) {
            advance();
            ASTNode* right = parseF();
            return parseTPrime(new BinaryOpNode('/', left, right));
        }
        return left;  // epsilon
    }

    // F -> ( E ) | id | num
    ASTNode* parseF() {
        if (currentToken.type == LPAREN) {
            advance();
            ASTNode* expr = parseE();
            expect(RPAREN);
            return expr;      // Parentheses don't create a node
        } else if (currentToken.type == ID) {
            string name = currentToken.value;
            advance();
            return new IdNode(name);
        } else if (currentToken.type == NUM) {
            string val = currentToken.value;
            advance();
            return new NumNode(val);
        }
        throw runtime_error("Syntax error in F");
    }
};


// ============================================================================
// HELPER: Parse and evaluate one expression
// ============================================================================
void evaluateExpression(const string& expr_str, const unordered_map<string, int>& env) {
    cout << "Expression: " << expr_str << "\n";
    try {
        Parser parser(expr_str);
        ASTNode* ast = parser.parse();

        cout << "AST Structure:\n";
        ast->print(2);

        cout << "Result: " << ast->evaluate(env) << "\n";
        cout << string(40, '-') << "\n";

        delete ast;   // Clean up (BinaryOpNode destructor deletes children)
    } catch (const exception& e) {
        cout << "Error: " << e.what() << "\n";
    }
}


// ============================================================================
// MAIN: Test cases
// ============================================================================
int main() {

    cout << "Given Grammar :" << endl;
    cout << "E -> TE'\nE' -> +TE' | -TE' | e\nT -> FT' \nT' -> *FT' | /FT' | e \nF -> (E) | id | num\n" << endl << endl;

    unordered_map<string, int> env = {
        {"id", 10},
        {"num", 2}
    };

    cout << "Evaluation Environment: id = 10, num = 2\n";
    cout << string(40, '-') << "\n";

    evaluateExpression("id + id * num", env);
    evaluateExpression("(id - num) / id", env);

    return 0;
}


// ============================================================================
// KEY DIFFERENCES: que1.cpp vs temp.cpp
// ============================================================================
//
// | Aspect           | que1.cpp (Struct)        | temp.cpp (OOP)              |
// |------------------|-------------------------|----------------------------|
// | Lexer            | Inline in parser        | Separate Lexer class       |
// | Tokens           | Direct string parsing   | Token stream (enum + val)  |
// | AST Nodes        | Single struct           | Class hierarchy (polymorphic) |
// | Evaluation       | Switch on node->value   | Virtual evaluate() per class |
// | Memory mgmt      | Manual (leaks!)         | RAII (destructors delete)  |
// | Error handling   | exit(1)                 | Exceptions (try/catch)     |
// | Environment      | Global map              | Passed as parameter        |
// | E' / T' impl     | while(true) loop        | Recursive function         |
//
// EXAM RELEVANCE:
// ---------------
// - que1.cpp style: Quick to write, shows core algorithm clearly
// - temp.cpp style: Production quality, shows software engineering
// - Both implement SAME grammar and produce SAME AST structure
// - Know BOTH patterns; exam might ask for either
//
// RECURSIVE E' vs ITERATIVE E':
// ------------------------------
// que1.cpp: while(true) { if(+) ... else break; }  // Iterative
// temp.cpp: parseEPrime(left) { if(+) return parseEPrime(new Op(...)); return left; } // Recursive
//
// Both produce LEFT-ASSOCIATIVE trees.
// Iterative is more efficient (no call stack), recursive mirrors grammar directly.
// ============================================================================