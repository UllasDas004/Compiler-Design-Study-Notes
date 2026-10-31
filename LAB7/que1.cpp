#include <iostream>
#include <string>
#include <map>
using namespace std;
enum NodeType{
	PROGRAM, DECL, ASSIGN, BINOP, ID, CONST
};
enum DataType{
	INT_T, FLOAT_T, VOID_T, ERR_T
};

struct ASTNode{
	NodeType nodeType;
	DataType dataType;
	string value;
	ASTNode *left;
	ASTNode *right;
};

ASTNode *makeNode(NodeType nt, DataType dt, string val, ASTNode *l, ASTNode *r)
{
	ASTNode *node = new ASTNode();
	node->nodeType = nt;
	node->dataType = dt;
	node->value = val;
	node->left = l;
	node->right = r;
	return node;
}

map<string, DataType> symbolTable;

DataType analyze(ASTNode *node){
	if (!node)
		return VOID_T;
	if (node->nodeType == PROGRAM){
		analyze(node->left);
		analyze(node->right);
		return VOID_T;
	}
	if (node->nodeType == DECL){
		if (symbolTable.find(node->value) != symbolTable.end()){
			cout << "Semantic Error: Redeclaration of variable '" << node->value << "'\n";
		}
		else{
			symbolTable[node->value] = node->dataType;
			cout << "Success: Declared '" << node->value << "'\n";
		}
		return VOID_T;
	}
	if (node->nodeType == ID){
		if (symbolTable.find(node->value) == symbolTable.end()){
			cout << "Semantic Error: Undeclared variable '" << node->value << "'\n";
			return ERR_T;
		}
		return symbolTable[node->value];
	}
	if (node->nodeType == CONST){
		return node->dataType;
	}
	if (node->nodeType == BINOP){
		DataType leftType  = analyze(node->left);
		DataType rightType = analyze(node->right);
		if (leftType == ERR_T || rightType == ERR_T)
			return ERR_T;
		if (leftType != rightType){
			cout << "Semantic Error: Type mismatch in '" << node->value << "' operation\n";
			return ERR_T;
		}
		return leftType;
	}
	if (node->nodeType == ASSIGN){
		DataType leftType = analyze(node->left);
		DataType rightType = analyze(node->right);
		if (leftType == ERR_T || rightType == ERR_T)
			return ERR_T;
		if (leftType != rightType){
			cout << "Semantic Error: Type mismatch in assignment to '" << node->left->value << "'\n";
			return ERR_T;
		}
		cout << "Success: Assignment to '" << node->left->value << "' is semantically valid.\n";
		return leftType;
	}
	return VOID_T;
}
int main(){
	ASTNode *n1 = makeNode(DECL, INT_T, "x", nullptr, nullptr);                   // int x;
	ASTNode *n2 = makeNode(ASSIGN, VOID_T, "=",
						   makeNode(ID, VOID_T, "x", nullptr, nullptr),           // x = 10;
						   makeNode(CONST, INT_T, "10", nullptr, nullptr));
	ASTNode *n3 = makeNode(DECL, FLOAT_T, "y", nullptr, nullptr);                 // float y;
	ASTNode *n4 = makeNode(ASSIGN, VOID_T, "=",                                   // y = x + 5.5;
						   makeNode(ID, VOID_T, "y", nullptr, nullptr),
						   makeNode(BINOP, VOID_T, "+",
									makeNode(ID, VOID_T, "x", nullptr, nullptr),
									makeNode(CONST, FLOAT_T, "5.5", nullptr, nullptr)));
	ASTNode *n5 = makeNode(ASSIGN, VOID_T, "=",                                    // z = 20;
						   makeNode(ID, VOID_T, "z", nullptr, nullptr),
						   makeNode(CONST, INT_T, "20", nullptr, nullptr));
	ASTNode *n6 = makeNode(DECL, INT_T, "x", nullptr, nullptr);                    // int x;
	ASTNode *p5 = makeNode(PROGRAM, VOID_T, "", n6, nullptr);
	ASTNode *p4 = makeNode(PROGRAM, VOID_T, "", n5, p5);
	ASTNode *p3 = makeNode(PROGRAM, VOID_T, "", n4, p4);
	ASTNode *p2 = makeNode(PROGRAM, VOID_T, "", n3, p3);
	ASTNode *p1 = makeNode(PROGRAM, VOID_T, "", n2, p2);
	ASTNode *root = makeNode(PROGRAM, VOID_T, "", n1, p1);
	
	analyze(root);
	return 0;
}
