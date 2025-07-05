
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

int my_flex_debug = 1;
#undef yy_flex_debug 
#define yy_flex_debug my_flex_debug

void yyerror(parse_location *loc, shader_lang_state* st, const char* msg);

#define TOKENIZE(TOK) tokenize(yylval->token, yylloc, (yytext), (yyleng), (TOK))

%}

/* 使生成的词法分析器是可重入的，即可以同时有多个实例 */
%option reentrant
/* 自动维护行号计数 */
%option yylineno
/* 生成与Bison协同工作的接口，特别是使`yylex`函数的签名包含一个额外的参数 */
%option bison-bridge
%option bison-locations noyywrap nounistd
%option never-interactive
%option debug
/*强制使用批处理模式，适用于yyin = fopen(...) %option batch */
/*%option prefix="thunder_"*/
%option extra-type="struct shader_lang_state *"

STRINGS										\"(\\.|[^\\"])*\"

BVECTOR					bool[1-4]
IVECTOR					int[1-4]
UVECTOR					uint[1-4]
HVECTOR					half[1-4]
FVECTOR					float[1-4]
DVECTOR					double[1-4]

VECTOR					{BVECTOR}|{IVECTOR}|{UVECTOR}|{HVECTOR}|{FVECTOR}|{DVECTOR}

BMATRIX					bool[1-4]x[1-4]
IMATRIX					int[1-4]x[1-4]
UMATRIX					uint[1-4]x[1-4]
HMATRIX					half[1-4]x[1-4]
FMATRIX					float[1-4]x[1-4]
DMATRIX					double[1-4]x[1-4]

MATRIX					{BMATRIX}|{IMATRIX}|{UMATRIX}|{HMATRIX}|{FMATRIX}|{DMATRIX}

TEXTURE_TAIL			((1D)(Array)?)|((2D)(MS)?(Array)?)|(3D)|((Cube)(Array)?)
SAMPLER_TAIL			(1D)|(2D)|(3D)|(CUBE)|(_state)|(State)|(ComparisonState)

TEXTURE					(t|T)exture{TEXTURE_TAIL}
SAMPLER					(s|S)ampler{SAMPLER_TAIL}

TEXCOORD				TEXCOORD[N|0-9]

TARGETS					[0-7]
SV_TARGET				SV_Target{TARGETS}?

%%

{VECTOR}		{ return TOKENIZE(TYPE_VECTOR); }
{MATRIX}		{ return TOKENIZE(TYPE_MATRIX); }
{TEXTURE}		{ return TOKENIZE(TYPE_TEXTURE); }
{SAMPLER}		{ return TOKENIZE(TYPE_SAMPLER); }

POSITION[0-7]?	{ return TOKENIZE(TOKEN_SV); }
SV_Position 	{ return TOKENIZE(TOKEN_SV); }
{TEXCOORD}  	{ return TOKENIZE(TOKEN_SV); }
{SV_TARGET} 	{ return TOKENIZE(TOKEN_SV); }

"Shader"		{ return TOKEN_SHADER; }
"Properties"	{ return TOKEN_PROPERTIES; }
"SubShader"		{ return TOKEN_SUBSHADER; }

"int"       	{ return TOKENIZE(TYPE_INT); }
"float"     	{ return TOKENIZE(TYPE_FLOAT); }
"void"      	{ return TOKENIZE(TYPE_VOID); }
"return"    	{ return TOKEN_RETURN; }
"struct"		{ return TOKEN_STRUCT; }

[a-zA-Z_][a-zA-Z0-9_]* { return TOKENIZE(TOKEN_IDENTIFIER); }
[0-9]+      	{ return TOKENIZE(TOKEN_INTEGER); }
{STRINGS}		{ return TOKENIZE(STRING_CONSTANT); }

"+"         	{ return ADD; }
"-"         	{ return SUB; }
"*"         	{ return MUL; }
"/"         	{ return DIV; }
"="         	{ return ASSIGN; }
":"				{ return COLON; }
";"         	{ return SEMICOLON; }
","         	{ return COMMA; }
"("         	{ return LPAREN; }
")"         	{ return RPAREN; }
"{"         	{ return LBRACE; }
"}"         	{ return RBRACE; }


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
