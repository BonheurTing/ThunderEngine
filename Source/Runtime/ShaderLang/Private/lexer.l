
%{
#define YY_NO_UNISTD_H   //阻止自动包含 <unistd.h>, 因为windows平台上不是标准 C/C++ 头文件

#include "parser.tab.h"
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

void yyerror(const char *);

/*告诉 Flex 不要依赖 unistd.h*/
//%option nounistd
/*禁用 yywrap 功能，有时也能避免 isatty 依赖*/
//
%}

%option never-interactive
%option debug
/*强制使用批处理模式，适用于yyin = fopen(...)*/
%option batch


%%

"int"       { return TYPE_INT; }
"float"     { return TYPE_FLOAT; }
"void"      { return TYPE_VOID; }
"return"    { return RETURN; }

[a-zA-Z_][a-zA-Z0-9_]* {
    yylval.strVal = strdup(yytext);
    return IDENTIFIER;
}

[0-9]+      {
    yylval.intVal = atoi(yytext);
    return INT_LITERAL;
}

"+"         { return ADD; }
"-"         { return SUB; }
"*"         { return MUL; }
"/"         { return DIV; }
"="         { return ASSIGN; }
";"         { return SEMICOLON; }
","         { return COMMA; }
"("         { return LPAREN; }
")"         { return RPAREN; }
"{"         { return LBRACE; }
"}"         { return RBRACE; }

[ \t\n]     ; // 忽略空白字符

.           { yyerror("Invalid character"); }
 
%%
 
int yywrap()
{
    return 1; /*1结束，0继续*/
}
