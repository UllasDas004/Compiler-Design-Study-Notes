%{
#include "math.tab.h"
%}

%%

[0-9]+(\.[0-9]+)? {
    return NUMBER;
}

[a-zA-Z_][a-zA-Z0-9_]* {
    return ID;
}

[-+*/()\[\]{}\n] {
    return yytext[0];
}

[ \t]+ {
    
}

. {
    return INVALID;
}

%%

int yywrap() {
    return 1;
}
