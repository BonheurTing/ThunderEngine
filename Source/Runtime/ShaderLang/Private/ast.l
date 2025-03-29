%{
#include "y.tab.h"
#include <string.h>
#include <stdlib.h>

#define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line; \
    yylloc.first_column = yylloc.last_column; \
    for (int i = 0; yytext[i]; i++) { \
    if (yytext[i] == '\n') { \
        yylloc.last_line++; \
        yylloc.last_column = 0; \
    } else { \
        yylloc.last_column++; \
    } \
    }

void yyerror(char *);
%}

%%

[0-9]+        { yylval.intVal = atoi(yytext); return NUMBER; }
[a-zA-Z][a-zA-Z0-9]* { yylval.strVal = strdup(yytext); return IDENTIFIER; }  // 变量
"\n"    { return EOL; }
[ \t]+  { /* 忽略空白字符 */ }
"+"     { return ADD; }
"-"     { return SUB; }
"*"     { return MUL; }
"/"     { return DIV; }
"("     { return LPAREN; }
")"     { return RPAREN; }
"="     { return ASSIGN; }
.       { fprintf(stderr, "lexical error at line %d: unexpected character '%s'\n", yylloc.first_line, yytext); return yytext[0]; }
 
%%
 
int yywrap()
{
    return 1;
}
