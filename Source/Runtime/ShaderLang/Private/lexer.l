
%{
#include "../Public/AstNode.h"
#include "../Public/ShaderLang.h"
using namespace Thunder;

#include "parser.tab.h"

#define YY_USER_ACTION \
	do { \
		yylloc->first_source = 1; \
		yylloc->last_source = 1; \
		yylloc->first_line = yylineno; \
		yylloc->last_line = yylineno; \
		yylloc->first_column = yycolumn; \
		yycolumn += (int)yyleng; \
		yylloc->last_column = yycolumn; \
	} while(0);


#define YY_USER_INIT \
	do { \
		yylineno = 1; \
		yycolumn = 1; \
	} while(0)

#define YYLTYPE parse_location

#define YYSTYPE parse_node

void yyerror(parse_location *loc, shader_lang_state* st, const char* msg);

%}

/* 使生成的词法分析器是可重入的，即可以同时有多个实例 */
%option reentrant
/* 生成与Bison协同工作的接口，特别是使`yylex`函数的签名包含一个额外的参数 */
%option bison-bridge
%option bison-locations noyywrap nounistd
%option never-interactive
%option debug
/*强制使用批处理模式，适用于yyin = fopen(...) %option batch */
/*%option prefix="thunder_"*/
%option extra-type="struct shader_lang_state *"

%%

"int"       { return TYPE_INT; }
"float"     { return TYPE_FLOAT; }
"void"      { return TYPE_VOID; }
"return"    { return RETURN; }

[a-zA-Z_][a-zA-Z0-9_]* {
    yylval->str_val = strdup(yytext);
    return IDENTIFIER;
}

[0-9]+      {
    yylval->int_val = atoi(yytext);
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

.           { /*yyerror("Invalid character");*/ }
 
%%

void lexer_constructor(struct shader_lang_state *state, const char* text)
{
	yylex_init_extra(state, &state->scanner);
	yy_scan_string(text, state->scanner);
}
void lexer_lexer_dtor(struct shader_lang_state *state)
{
	yylex_destroy(state->scanner);
}
