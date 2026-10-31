// =============================================================================
// LAB 8: Three-Address Code (TAC) Generation
// =============================================================================
// QUESTION:
// Write a program that accepts an arithmetic expression or a simple assignment
// statement (e.g., a = b + c * d).
//
// (a) Performs lexical analysis and parsing (recursive descent parser).
// (b) Generates Three-Address Code (TAC) for the given expression.
// (c) Converts the TAC into Quadruple and Triple representations.
// (d) Handles basic operators: +, -, *, /, and assignment =.
// (e) Uses temporary variables (e.g., t1, t2, …) for intermediate results.
//
// =============================================================================
// HOW IT WORKS (Big Picture):
//
// INPUT: "a = b + c * d"
//
// STEP 1 - LEXER: Breaks input into tokens
//   "a"  -> ID token
//   "="  -> ASSIGN token
//   "b"  -> ID token
//   "+"  -> OP token
//   "c"  -> ID token
//   "*"  -> OP token
//   "d"  -> ID token
//
// STEP 2 - PARSER (Recursive Descent):
//   Respects operator precedence: * before +
//   Generates temporaries bottom-up:
//     t1 = c * d     (multiply first, higher precedence)
//     t2 = b + t1    (then add)
//     a = t2         (final assignment)
//
// STEP 3 - Output Formats:
//   TAC:        t1 = c * d
//   Quadruple:  (*, c, d, t1)
//   Triple:     (0) *, c, d      -- no result field, use index to refer
// =============================================================================

#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <iomanip>
#include <map>
using namespace std;

// =============================================================================
// TOKEN TYPES
// =============================================================================
enum TokenType { ID, NUM, OP, ASSIGN, LPAREN, RPAREN, END };

struct Token {
    TokenType type;
    string value;
};

// =============================================================================
// LEXER: Breaks input string into tokens
// =============================================================================
class Lexer {
    string input;  // the full input string
    size_t pos;    // current position in the string

public:
    Lexer(string str) : input(str), pos(0) {}

    Token getNextToken() {
        // Skip whitespace
        while (pos < input.length() && isspace(input[pos]))
            pos++;

        // End of input
        if (pos >= input.length())
            return {END, ""};

        char c = input[pos];

        // Identifier: starts with a letter, can contain letters/digits
        if (isalpha(c)) {
            string id = "";
            while (pos < input.length() && isalnum(input[pos]))
                id += input[pos++];
            return {ID, id};
        }

        // Number: sequence of digits
        if (isdigit(c)) {
            string num = "";
            while (pos < input.length() && isdigit(input[pos]))
                num += input[pos++];
            return {NUM, num};
        }

        pos++; // consume the single character

        // Assignment operator
        if (c == '=')  return {ASSIGN, "="};

        // Arithmetic operators
        if (c == '+' || c == '-' || c == '*' || c == '/')
            return {OP, string(1, c)};

        // Parentheses
        if (c == '(')  return {LPAREN, "("};
        if (c == ')')  return {RPAREN, ")"};

        // Unknown character - skip
        return {END, ""};
    }

    // Tokenize the entire input into a vector of tokens
    vector<Token> tokenize() {
        vector<Token> tokens;
        Token t;
        do {
            t = getNextToken();
            if (t.type != END)
                tokens.push_back(t);
        } while (t.type != END);
        return tokens;
    }
};

// =============================================================================
// THREE-ADDRESS CODE STRUCTURE
// A single TAC instruction: result = arg1 op arg2
// For assignment only:      result = arg1  (arg2 is empty)
// =============================================================================
struct TAC {
    string op;     // operator: +, -, *, /, = 
    string arg1;   // left operand
    string arg2;   // right operand (empty for simple assignment)
    string result; // the variable that stores the result
};

// =============================================================================
// PARSER: Recursive Descent Parser
//
// Grammar (respects operator precedence):
//   Statement  -> ID = Expression  |  Expression
//   Expression -> Term ((+ | -) Term)*      [lower precedence]
//   Term       -> Factor ((* | /) Factor)*  [higher precedence]
//   Factor     -> ( Expression ) | ID | NUM
//
// Key idea: * and / bind tighter than + and -, so we parse them
// in separate functions. parseTerm() handles * and /, and
// parseExpression() handles + and -.
// =============================================================================
class Parser {
    vector<Token> tokens;  // all tokens from lexer
    size_t pos;            // current token position
    int tempCount;         // counter for generating t1, t2, t3, ...
    vector<TAC> tacList;   // stores all generated TAC instructions

    // Generate a new unique temporary variable name
    string newTemp() {
        return "t" + to_string(tempCount++);
    }

    // Add a new TAC instruction to the list
    void emit(string op, string arg1, string arg2, string result) {
        tacList.push_back({op, arg1, arg2, result});
    }

    // ------------------------------------------------------------------
    // Factor -> '(' Expression ')' | ID | NUM
    // This is the base case - handles literals and grouped expressions
    // ------------------------------------------------------------------
    string parseFactor() {
        // Parenthesized expression: (E)
        if (pos < tokens.size() && tokens[pos].type == LPAREN) {
            pos++; // consume '('
            string res = parseExpression();
            if (pos < tokens.size() && tokens[pos].type == RPAREN)
                pos++; // consume ')'
            return res;
        }

        // ID or NUM - just return the variable/number name as a string
        if (pos < tokens.size() && (tokens[pos].type == ID || tokens[pos].type == NUM)) {
            return tokens[pos++].value;
        }

        return ""; // error case
    }

    // ------------------------------------------------------------------
    // Term -> Factor ((* | /) Factor)*
    // Handles multiplication and division (higher precedence)
    // ------------------------------------------------------------------
    string parseTerm() {
        string left = parseFactor(); // parse the first factor

        // Keep consuming * or / operators
        while (pos < tokens.size() && tokens[pos].type == OP &&
               (tokens[pos].value == "*" || tokens[pos].value == "/")) {

            string op = tokens[pos++].value; // get the operator
            string right = parseFactor();     // parse right factor
            string res = newTemp();           // create a new temp var
            emit(op, left, right, res);       // emit: res = left op right
            left = res;                       // result becomes new left
        }

        return left;
    }

    // ------------------------------------------------------------------
    // Expression -> Term ((+ | -) Term)*
    // Handles addition and subtraction (lower precedence)
    // ------------------------------------------------------------------
    string parseExpression() {
        string left = parseTerm(); // parse the first term

        // Keep consuming + or - operators
        while (pos < tokens.size() && tokens[pos].type == OP &&
               (tokens[pos].value == "+" || tokens[pos].value == "-")) {

            string op = tokens[pos++].value; // get the operator
            string right = parseTerm();       // parse right term
            string res = newTemp();           // create a new temp var
            emit(op, left, right, res);       // emit: res = left op right
            left = res;                       // result becomes new left
        }

        return left;
    }

public:
    Parser(vector<Token> t) : tokens(t), pos(0), tempCount(1) {}

    // ------------------------------------------------------------------
    // parseStatement: Entry point
    // Checks if it's an assignment (ID = ...) or just an expression
    // ------------------------------------------------------------------
    void parseStatement() {
        // Check for assignment: first token is ID, second is '='
        if (tokens.size() > 1 && tokens[0].type == ID && tokens[1].type == ASSIGN) {
            string target = tokens[0].value; // the variable on the left of =
            pos = 2;                          // skip the ID and = tokens
            string exprResult = parseExpression(); // parse the right side
            emit("=", exprResult, "", target); // emit: target = exprResult
        }
        else {
            // Just an expression, no assignment
            parseExpression();
        }
    }

    // ------------------------------------------------------------------
    // PRINT: Three-Address Code (TAC)
    // Format: result = arg1 op arg2
    // ------------------------------------------------------------------
    void printTAC() {
        cout << "\n--- Three-Address Code (TAC) ---\n";
        for (const auto& t : tacList) {
            if (t.op == "=")
                cout << t.result << " = " << t.arg1 << "\n";
            else
                cout << t.result << " = " << t.arg1 << " " << t.op << " " << t.arg2 << "\n";
        }
    }

    // ------------------------------------------------------------------
    // PRINT: Quadruples
    // Format: (operator, arg1, arg2, result)
    // Stores 4 fields per instruction - very explicit
    // Unlike triples, result field explicitly names the temp variable
    // ------------------------------------------------------------------
    void printQuadruples() {
        cout << "\n--- Quadruples ---\n";
        cout << left << setw(5) << "#"
             << setw(10) << "Op"
             << setw(10) << "Arg1"
             << setw(10) << "Arg2"
             << "Result\n";
        cout << string(45, '-') << "\n";

        for (int i = 0; i < tacList.size(); i++) {
            const TAC& t = tacList[i];
            cout << left << setw(5) << i
                 << setw(10) << t.op
                 << setw(10) << t.arg1
                 << setw(10) << (t.arg2.empty() ? "-" : t.arg2)
                 << t.result << "\n";
        }
    }

    // ------------------------------------------------------------------
    // PRINT: Triples
    // Format: (index) operator, arg1, arg2
    // NO result field - result is referred to by the instruction's index
    // e.g., "(0)" means "use the result of instruction 0"
    // This saves space but makes optimization harder (can't move instrs)
    // ------------------------------------------------------------------
    void printTriples() {
        cout << "\n--- Triples ---\n";
        cout << left << setw(8) << "Index"
             << setw(10) << "Op"
             << setw(12) << "Arg1"
             << "Arg2\n";
        cout << string(42, '-') << "\n";

        // Build a map: temp variable name -> instruction index that computed it
        // e.g., "t1" -> 0, "t2" -> 1, etc.
        map<string, int> tempToIndex;
        for (int i = 0; i < tacList.size(); i++) {
            // If the result starts with 't', it's a temporary
            if (!tacList[i].result.empty() && tacList[i].result[0] == 't')
                tempToIndex[tacList[i].result] = i;
        }

        for (int i = 0; i < tacList.size(); i++) {
            string arg1 = tacList[i].arg1;
            string arg2 = tacList[i].arg2;

            // Replace temp variable names with their index references
            // e.g., "t1" becomes "(0)" meaning "result of instruction 0"
            if (tempToIndex.count(arg1))
                arg1 = "(" + to_string(tempToIndex[arg1]) + ")";
            if (tempToIndex.count(arg2))
                arg2 = "(" + to_string(tempToIndex[arg2]) + ")";

            cout << left << "(" << i << ")" << setw(5) << ""
                 << setw(10) << tacList[i].op
                 << setw(12) << arg1;

            // For assignment (=), show the target variable
            if (tacList[i].op == "=")
                cout << tacList[i].result << " (Target)\n";
            else
                cout << (arg2.empty() ? "-" : arg2) << "\n";
        }
    }
};

// =============================================================================
// MAIN: Entry point
// =============================================================================
int main() {
    string expression;
    cout << "Enter an arithmetic expression or assignment\n";
    cout << "Examples: a + b * c  OR  a = b + c * d  OR  (a + b) * c\n";
    cout << "> ";
    getline(cin, expression);

    cout << "\n=== Input: " << expression << " ===\n";

    // Step 1: Tokenize
    Lexer lexer(expression);
    vector<Token> tokens = lexer.tokenize();

    // Show tokens
    cout << "\n--- Tokens ---\n";
    for (const auto& t : tokens) {
        string typeName;
        switch(t.type) {
            case ID:     typeName = "ID";     break;
            case NUM:    typeName = "NUM";    break;
            case OP:     typeName = "OP";     break;
            case ASSIGN: typeName = "ASSIGN"; break;
            case LPAREN: typeName = "LPAREN"; break;
            case RPAREN: typeName = "RPAREN"; break;
            default:     typeName = "END";    break;
        }
        cout << "[" << typeName << ", " << t.value << "]\n";
    }

    // Step 2: Parse and generate TAC
    Parser parser(tokens);
    parser.parseStatement();

    // Step 3: Print all three representations
    parser.printTAC();
    parser.printQuadruples();
    parser.printTriples();

    return 0;
}
