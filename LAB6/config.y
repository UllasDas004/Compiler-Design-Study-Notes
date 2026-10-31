%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int yylex();
void yyerror(const char *s);

char vars[100][50];
int vals[100];
int var_count = 0;

int get_var(char *name) {
	for(int i = 0; i < var_count; i++) {
		if(strcmp(vars[i], name) == 0) {
			return vals[i];
		}
	}
	return 0;
}

void set_var(char *name, int val) {
	for(int i = 0; i < var_count; i++) {
		if(strcmp(vars[i], name) == 0) {
			vals[i] = val;
			return;
		}
	}
	strcpy(vars[var_count], name);
	vals[var_count] = val;
	var_count++;
}
%}

%union {
	int num;
	char* str;
}

%token <str> IDENTIFIER
%token <num> NUMBER
%type <num> expr

%left '+' '-'
%left '*' '/'

%%
program:
	program statement '\n'
	| program '\n'
	| statement '\n'
	| '\n'
	;
statement:
	IDENTIFIER '=' expr ';' {
		set_var($1, $3);
		printf("Success: Assigned %s = %d\n", $1, $3);
		free($1);
	}
	| expr ';' {
		printf("Evaluated Result: %d\n", $1);
	}
	;
expr:
	NUMBER              { $$ = $1; }
	| IDENTIFIER        { $$ = get_var($1); free($1); }
	| expr '+' expr     { $$ = $1 + $3; }
	| expr '-' expr     { $$ = $1 - $3; }
	| expr '*' expr     { $$ = $1 * $3; }
	| expr '/' expr     { $$ = $1 / $3; }
	| '(' expr ')'      { $$ = $2; }
	;
%%

void yyerror(const char *s) {
	fprintf(stderr, "Syntax Error: %s\n", s);
}

int main() {
	yyparse();
	return 0;
}
