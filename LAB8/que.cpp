 #include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <iomanip>
#include <map>
using namespace std;

enum TokenType{
	ID,
	NUM,
	OP,
	ASSIGN,
	LPAREN,
	RPAREN,
	END
};
struct Token{
	TokenType type;
	string value;
};
class Lexer{
	string input;
	size_t pos;

public:
	Lexer(string str) : input(str), pos(0) {}
	Token getNextToken(){
		while (pos < input.length() && isspace(input[pos]))
			pos++;
		if (pos >= input.length())
			return {END, ""};
		char current = input[pos];
		if (isalpha(current)){
			string id = "";
			while (pos < input.length() && isalnum(input[pos])){
				id += input[pos++];
			}
			return {ID, id};
		}
		if (isdigit(current)){
			string num = "";
			while (pos < input.length() && isdigit(input[pos])){
				num += input[pos++];
			}
			return {NUM, num};
		}
		if (current == '='){
			pos++;
			return {ASSIGN, "="};
		}
		if (current == '+' || current == '-' || current == '*' || current == '/'){
			pos++;
			return {OP, string(1, current)};
		}
		if (current == '('){
			pos++;
			return {LPAREN, "("};
		}
		if (current == ')'){
			pos++;
			return {RPAREN, ")"};
		}
		pos++;
		return {END, ""};
	}

	vector<Token> tokenize(){
		vector<Token> tokens;
		Token t;
		do{
			t = getNextToken();
			if (t.type != END)
				tokens.push_back(t);
		} while (t.type != END);
		return tokens;
	}
};

struct TAC{
	string op;
	string arg1;
	string arg2;
	string result;
};
class Parser{
	vector<Token> tokens;
	size_t pos;
	int tempCount;
	vector<TAC> tacList;
	string newTemp(){
		return "t" + to_string(tempCount++);
	}
	void emit(string op, string arg1, string arg2, string result){
		tacList.push_back({op, arg1, arg2, result});
	}

	bool match(TokenType type){
		if (pos < tokens.size() && tokens[pos].type == type){
			pos++;
			return true;
		}
		return false;
	}

	string parseFactor(){     // Factor -> '(' Expression ')' | ID | NUM
		if (pos < tokens.size() && tokens[pos].type == LPAREN){
			pos++; 
			string res = parseExpression();
			if (pos < tokens.size() && tokens[pos].type == RPAREN)
				pos++; 
			return res;
		}
		else if (pos < tokens.size() && (tokens[pos].type == ID || tokens[pos].type == NUM)){
			return tokens[pos++].value;
		}
		return "";
	}

	string parseTerm(){       // Term -> Factor ((* | /)Factor)*
		string left = parseFactor();
		while (pos < tokens.size() && tokens[pos].type == OP &&
			   (tokens[pos].value == "*" || tokens[pos].value == "/")){
			string op = tokens[pos++].value;
			string right = parseFactor();
			string res = newTemp();
			emit(op, left, right, res);
			left = res;
		}
		return left;
	}

	string parseExpression(){        // Expression -> Term ((+ | -) Term)*
		string left = parseTerm();
		while (pos < tokens.size() && tokens[pos].type == OP &&
			   (tokens[pos].value == "+" || tokens[pos].value == "-")){
			string op = tokens[pos++].value;
			string right = parseTerm();
			string res = newTemp();
			emit(op, left, right, res);
			left = res;
		}
		return left;
	}

public:
	Parser(vector<Token> t) : tokens(t), pos(0), tempCount(1) {}

	void parseStatement(){
		if (tokens.size() > 1 && tokens[0].type == ID && tokens[1].type == ASSIGN){
			string target = tokens[0].value;
			pos = 2;
			string exprResult = parseExpression();
			emit("=", exprResult, "", target);
		}
		else{
			parseExpression();
		}
	}

	void printTAC(){
		cout << "\n--- Three-Address Code (TAC) ---\n";
		for (const auto &t : tacList){
			if (t.op == "="){
				cout << t.result << " = " << t.arg1 << endl;
			}
			else{
				cout << t.result << " = " << t.arg1 << " " << t.op << " " << t.arg2 << endl;
			}
		}
	}
	void printQuadruples(){
		cout << "\n--- Quadruples ---\n";
		cout << left << setw(10) << "Operator" << setw(10) << "Arg1" << setw(10) << "Arg2" << "Result\n";
		cout << "------------------------------------------\n";
		for (const auto &t : tacList)
		{
			cout << left << setw(10) << t.op << setw(10) << t.arg1 << setw(10) << (t.arg2.empty() ? "-" : t.arg2) << t.result << endl;
		}
	}
	void printTriples(){
		cout << "\n--- Triples ---\n";
		cout << left << setw(10) << "Index" << setw(10) << "Operator" << setw(10) << "Arg1"
			 << "Arg2\n";
		cout << "------------------------------------------\n";
		map<string, int> tempToIndex;
		for (size_t i = 0; i < tacList.size(); i++){
			if (tacList[i].result[0] == 't'){
				tempToIndex[tacList[i].result] = i;
			}
		}
		for (size_t i = 0; i < tacList.size(); i++){
			string arg1 = tacList[i].arg1;
			string arg2 = tacList[i].arg2;
			if (tempToIndex.count(arg1))
				arg1 = "(" + to_string(tempToIndex[arg1]) + ")";
			if (tempToIndex.count(arg2))
				arg2 = "(" + to_string(tempToIndex[arg2]) + ")";
				cout
				<< left << "(" << i << ")" << setw(7) << "" << setw(10) << tacList[i].op << setw(10) << arg1;
			if (tacList[i].op == "="){
				cout << tacList[i].result << " (Target)\n";
			}
			else{
				cout << (arg2.empty() ? "-" : arg2) << endl;
			}
		}
	}
};
int main(){
	string expression;
	cout << "Enter an arithmetic expression or assignment : ";
	getline(cin, expression);
	Lexer lexer(expression);
	vector<Token> tokens = lexer.tokenize();
	Parser parser(tokens);
	parser.parseStatement();
	parser.printTAC();
	parser.printQuadruples();
	parser.printTriples();
	return 0;
}
