To design and implement a syntax checker and evaluator for a basic configuration language or
subset of a scripting language that supports variable assignments and dynamic variable evaluation
using the Bision parser generator and Flex lexical analyser.

%{
#include "config.tab.h"
#include <string.h>
#include <stdlib.h>
%}

%%

[a-zA-Z_][a-zA-Z0-9_]* {
    yylval.str = strdup(yytext);
    return IDENTIFIER;
}

[0-9]+ {
    yylval.num = atoi(yytext);
    return NUMBER;
}

[-+*/=;\n] {
    return yytext[0];
}

[ \t]+ {
  
}

. {
   
}

%%

int yywrap() {
    return 1;
}

