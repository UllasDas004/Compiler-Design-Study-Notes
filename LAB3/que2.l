// Write a Lex (or Flex) program
// that recognizes tokens for a small
// programming language fragment and
// prints each token along with its type and lexeme.

%{
#include <stdio.h>
%}

DIGIT    [0-9]
ID       [a-zA-Z_][a-zA-Z0-9_]*

%%* num

"int"|"float"|"if"|"else"|"return" { printf("KEYWORD: %s\n", yytext); }

{ID}        { printf("IDENTIFIER: %s\n", yytext); }

{DIGIT}+    { printf("NUMBER (INTEGER): %s\n", yytext); }

"+"|"-"|"*"|"/" { printf("ARITHMETIC OPERATOR: %s\n", yytext); }
"="         { printf("ASSIGNMENT OPERATOR: %s\n", yytext); }
"=="|"!="   { printf("RELATIONAL OPERATOR: %s\n", yytext); }
";"|","     { printf("PUNCTUATOR: %s\n", yytext); }

[ \t\n]+    {}

.           { printf("UNKNOWN TOKEN: %s\n", yytext); }

%%

int yywrap() {
    return 1;
}

int main() {
    printf("Enter code (Press Ctrl+D to stop):\n");
    yylex();
    return 0;
}