#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <stdexcept>
#include <iomanip>

using namespace std;

enum TokenType
{
    TOKEN_ID,
    TOKEN_NUM,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MUL,
    TOKEN_DIV,
    TOKEN_ASSIGN,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_EOF
};

struct Token
{
    TokenType type;
    string value;
};

class Lexer
{
private:
    string input;
    size_t pos;

public:
    Lexer(const string& src) : input(src), pos(0) {}

    Token getNextToken()
    {
        while(pos < input.length() && isspace(input[pos])) pos++;

        if(pos >= input.length()) return {TOKEN_EOF, ""};

        char current = input[pos];

        if(isdigit(current))
        {
            string num = "";
            while(pos < input.length() && (isdigit(input[pos]) || input[pos] == '.')) num += input[pos++];
            return {TOKEN_NUM, num};
        }

        if (isalpha(current))
        {
            string id = "";
            while(pos < input.length() && isalnum(input[pos])) id += input[pos++];
            return {TOKEN_ID, id};
        }

        switch (current)
        {
            case '+': pos++; return {TOKEN_PLUS, "+"};
            case '-': pos++; return {TOKEN_MINUS, "-"};
            case '*': pos++; return {TOKEN_MUL, "*"};
            case '/': pos++; return {TOKEN_DIV, "/"};
            case '=': pos++; return {TOKEN_ASSIGN, "="};
            case '(': pos++; return {TOKEN_LPAREN, "("};
            case ')': pos++; return {TOKEN_RPAREN, ")"};
            default:
                throw runtime_error(string("Lexical Error: Unknown character '") + current + "'");
        }
    }

    vector<Token> tokenize()
    {
        vector<Token> tokens;
        Token t;
        do
        {
            t = getNextToken();
            tokens.push_back(t);
        } while (t.type != TOKEN_EOF);
        return tokens;
    }
};

struct TAC
{
    string op;
    string arg1;
    string arg2;
    string result;
};

class Parser
{
private:
    vector<Token> tokens;
    size_t pos;
    int tempCounter;
    vector<TAC> tacInstructions;

    string newTemp()
    {
        return "t" + to_string(tempCounter++);
    }

    void emit(string op, string arg1, string arg2, string result)
    {
        tacInstructions.push_back({op, arg1, arg2, result});
    }

    string parseFactor()
    {
        Token t = tokens[pos];
        if(t.type == TOKEN_ID || t.type == TOKEN_NUM)
        {
            pos++;
            return t.value;
        }
        else if (t.type == TOKEN_LPAREN)
        {
            pos++;
            string result = parseExpression();
            if(tokens[pos].type == TOKEN_RPAREN)
            {
                pos++;
            }
            else
            {
                throw runtime_error("Syntax Error: Expected ')'");
            }
            return result;
        }
        throw runtime_error("Syntax Error: Expected Identifier, Number, or '('");
    }

    string parseTerm()
    {
        string left = parseFactor();

        while (tokens[pos].type == TOKEN_MUL || tokens[pos].type == TOKEN_DIV)
        {
            string op = tokens[pos].value;
            pos++;
            string right = parseFactor();
            
            string result = newTemp();
            emit(op, left, right, result);
            left = result;
        }
        return left;
    }

    string parseExpression()
    {
        string left = parseTerm();

        while(tokens[pos].type == TOKEN_PLUS || tokens[pos].type == TOKEN_MINUS)
        {
            string op = tokens[pos].value;
            pos++;
            string right = parseTerm();
            
            string result = newTemp();
            emit(op, left, right, result);
            left = result;
        }
        return left;
    }

public:
    Parser(vector<Token> t) : tokens(t), pos(0), tempCounter(1) {}

    void parseStatement()
    {
        if(tokens[pos].type == TOKEN_ID && pos + 1 < tokens.size() && tokens[pos + 1].type == TOKEN_ASSIGN)
        {
            string target = tokens[pos].value;
            pos += 2;
            
            string exprResult = parseExpression();
            
            emit("=", exprResult, "", target);
        }
        else
            parseExpression();

        if(tokens[pos].type != TOKEN_EOF)
        {
            throw runtime_error("Syntax Error: Unexpected tokens at end of statement.");
        }
    }

    vector<TAC> getTAC()
    {
        return tacInstructions;
    }
};

void printThreeAddressCode(const vector<TAC>& tacList)
{
    cout << "\n--- Three-Address Code (TAC) ---\n";
    for(const auto& tac : tacList)
    {
        if(tac.op == "=") cout << tac.result << " = " << tac.arg1 << endl;
        else cout << tac.result << " = " << tac.arg1 << " " << tac.op << " " << tac.arg2 << endl;
    }
}

void printQuadruples(const vector<TAC>& tacList)
{
    cout << "\n--- Quadruple Representation ---\n";
    cout << left << setw(10) << "Operator" << setw(10) << "Arg1" << setw(10) << "Arg2" << setw(10) << "Result" << endl;
    cout << "----------------------------------------\n";
    for(const auto& tac : tacList)
    {
        string arg2 = (tac.arg2 == "") ? "-" : tac.arg2;
        cout << left << setw(10) << tac.op 
             << setw(10) << tac.arg1 
             << setw(10) << arg2 
             << setw(10) << tac.result << endl;
    }
}

void printTriples(const vector<TAC>& tacList)
{
    cout << "\n--- Triple Representation ---\n";
    cout << left << setw(10) << "Index" << setw(10) << "Operator" << setw(15) << "Arg1" << setw(15) << "Arg2" << endl;
    cout << "--------------------------------------------------\n";
    
    map<string, string> tempToLineMap;

    for (size_t i = 0; i < tacList.size(); i++)
    {
        const auto& tac = tacList[i];
        
        string a1 = (tempToLineMap.count(tac.arg1) > 0) ? tempToLineMap[tac.arg1] : tac.arg1;
        string a2 = (tempToLineMap.count(tac.arg2) > 0) ? tempToLineMap[tac.arg2] : tac.arg2;

        if (tac.op == "=")
        {
            cout << left << "(" << i << ")" << setw(7) << " "
                 << setw(10) << "=" 
                 << setw(15) << tac.result 
                 << setw(15) << a1 << endl;
        }
        else
        {
            cout << left << "(" << i << ")" << setw(7) << " "
                 << setw(10) << tac.op 
                 << setw(15) << a1 
                 << setw(15) << a2 << endl;
            
            tempToLineMap[tac.result] = "(" + to_string(i) + ")";
        }
    }
}

int main() {
    string input_expr;
    cout << "Enter an expression or assignment:\n> ";
    getline(cin, input_expr);

    if (input_expr.empty())
    {
        cout << "Empty input. Exiting...\n";
        return 0;
    }

    try
    {
        Lexer lexer(input_expr);
        vector<Token> tokens = lexer.tokenize();
        for(auto& token : tokens) cout<<token.value<<" "<<token.type<<endl;

        Parser parser(tokens);
        parser.parseStatement();
        
        vector<TAC> tacList = parser.getTAC();

        printThreeAddressCode(tacList);
        printQuadruples(tacList);
        printTriples(tacList);
        

    }
    catch (const exception& e)
    {
        cerr << "\n[Error] " << e.what() << endl;
    }

    return 0;
}