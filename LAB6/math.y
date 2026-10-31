%{
#include <stdio.h>
#include <stdlib.h>

int yylex();
void yyerror(const char *s);

int valid = 1;
%}

%token NUMBER ID INVALID

%left '+' '-'
%left '*' '/'

%%

input:
      input line
    | line
    ;

line:
      expr '\n' {
          if (valid) {
              printf("Valid: Correct mathematical structure and balanced symbols.\n");
          }
          valid = 1;
      }
    | error '\n' {
          valid = 1;
          yyerrok;
      }
    | '\n'
    ;

expr:
      NUMBER
    | ID
    | expr '+' expr
    | expr '-' expr
    | expr '*' expr
    | expr '/' expr
    | '(' expr ')'
    | '[' expr ']'
    | '{' expr '}'
    ;

%%

void yyerror(const char *s) {
    printf("Syntax Error: Invalid expression or unbalanced parentheses/brackets.\n");
    valid = 0;
}

int main() {
    yyparse();
    return 0;
}
