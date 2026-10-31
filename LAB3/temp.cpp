#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cctype>
#include <stdexcept>

using namespace std;

enum TokenType { ID, NUM, PLUS, MINUS, MUL, DIV, LPAREN, RPAREN, END };

struct Token {
    TokenType type;
    string value;
};

class Lexer {
    string text;
    size_t pos;
public:
    Lexer(const string& input) : text(input), pos(0) {}

    Token getNextToken() {
        while (pos < text.length() && isspace(text[pos])) pos++;

        if (pos >= text.length()) return {END, ""};

        char current = text[pos];

        if (isalpha(current)) {
            string id = "";
            while (pos < text.length() && isalnum(text[pos])) {
                id += text[pos++];
            }

            if (id == "num") return {NUM, id};
            return {ID, id};
        }
        if (isdigit(current)) {
            string num = "";
            while (pos < text.length() && isdigit(text[pos])) {
                num += text[pos++];
            }
            return {NUM, num};
        }

        pos++;
        switch (current) {
            case '+': return {PLUS, "+"};
            case '-': return {MINUS, "-"};
            case '*': return {MUL, "*"};
            case '/': return {DIV, "/"};
            case '(': return {LPAREN, "("};
            case ')': return {RPAREN, ")"};
            default: throw runtime_error(string("Unknown character: ") + current);
        }
    }
};

class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual int evaluate(const unordered_map<string, int>& env) = 0;
    virtual void print(int indent = 0) = 0;
};

class IdNode : public ASTNode {
    string name;
public:
    IdNode(const string& n) : name(n) {}
    int evaluate(const unordered_map<string, int>& env) override {
        return env.at(name);
    }
    void print(int indent) override {
        cout << string(indent, ' ') << "ID(" << name << ")" << endl;
    }
};

class NumNode : public ASTNode {
    string value;
public:
    NumNode(const string& v) : value(v) {}
    int evaluate(const unordered_map<string, int>& env) override {
        if (value == "num") return env.at("num");
        return stoi(value);
    }
    void print(int indent) override {
        cout << string(indent, ' ') << "NUM(" << value << ")" << endl;
    }
};

class BinaryOpNode : public ASTNode {
    char op;
    ASTNode* left;
    ASTNode* right;
public:
    BinaryOpNode(char o, ASTNode* l, ASTNode* r) : op(o), left(l), right(r) {}
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

class Parser {
    Lexer lexer;
    Token currentToken;

    void advance() {
        currentToken = lexer.getNextToken();
    }

    bool match(TokenType type) {
        if (currentToken.type == type) {
            advance();
            return true;
        }
        return false;
    }

    void expect(TokenType type) {
        if (!match(type)) {
            throw runtime_error("Syntax error: Unexpected token");
        }
    }

public:
    Parser(const string& text) : lexer(text) {
        advance();
    }

    ASTNode* parse() {
        ASTNode* ast = parseE();
        if (currentToken.type != END) {
            throw runtime_error("Syntax error: Unexpected characters at end");
        }
        return ast;
    }

    // E -> T E'
    ASTNode* parseE() {
        ASTNode* left = parseT();
        return parseEPrime(left);
    }

    // E' -> + T E' | - T E' | epsilon
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
        return left; // epsilon
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
        return left; // epsilon
    }

    // F -> ( E ) | id | num
    ASTNode* parseF() {
        if (currentToken.type == LPAREN) {
            advance();
            ASTNode* expr = parseE();
            expect(RPAREN);
            return expr;
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

void evaluateExpression(const string& expr_str, const unordered_map<string, int>& env) {
    cout << "Expression: " << expr_str << "\n";
    try {
        Parser parser(expr_str);
        ASTNode* ast = parser.parse();
       
        cout << "AST Structure:\n";
        ast->print(2);
       
        cout << "Result: " << ast->evaluate(env) << "\n";
        cout << string(40, '-') << "\n";
       
        delete ast;
    } catch (const exception& e) {
        cout << "Error: " << e.what() << "\n";
    }
}

int main() {

	cout<<"Given Grammer :" <<endl;
	cout<<"E -> TE'\nE' -> +TE' | -TE' | e\nT -> FT' \nT' -> *FT' | /FT' | e \nF -> (E) | id | num\n"<<endl<<endl;
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

