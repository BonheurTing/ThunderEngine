
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
#define TOKENIZE_COMMENT()

%}

%option reentrant
%option yylineno
%option bison-bridge
%option bison-locations noyywrap nounistd
%option never-interactive
%option debug
/*%option prefix="thunder_"*/
%option extra-type="struct shader_lang_state *"

STRINGS					\"(\\.|[^\\"])*\"

ANY_BUTSTAR				[^\*]
ANY_BUTSLASH			[^\/]
COMMENT					[\/][\*]({ANY_BUTSTAR}|[\*]{ANY_BUTSLASH})*[\*][\/]
FASTCOMMENT				[\/][\/].*

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
TEXTURE					(t|T)exture{TEXTURE_TAIL}

TEXCOORD				TEXCOORD[N|0-9]
Texcoord				Texcoord[N|0-9]

TARGETS					[0-7]
SV_TARGET				SV_TARGET{TARGETS}?
SV_Target				SV_Target{TARGETS}?

%%

"Buffer"					{ return TOKENIZE(TYPE_OBJECT); }
"ByteAddressBuffer"			{ return TOKENIZE(TYPE_OBJECT); }
"StructuredBuffer"			{ return TOKENIZE(TYPE_OBJECT); }
"AppendStructuredBuffer"	{ return TOKENIZE(TYPE_OBJECT); }
"ConsumeStructuredBuffer"	{ return TOKENIZE(TYPE_OBJECT); }
"RWBuffer"					{ return TOKENIZE(TYPE_OBJECT); }
"RWByteAddressBuffer"		{ return TOKENIZE(TYPE_OBJECT); }
"RWStructuredBuffer"		{ return TOKENIZE(TYPE_OBJECT); }

"Texture1D"					{ return TOKENIZE(TYPE_OBJECT); }
"Texture1DArray"			{ return TOKENIZE(TYPE_OBJECT); }
"Texture2D"					{ return TOKENIZE(TYPE_OBJECT); }
"Texture2DArray"			{ return TOKENIZE(TYPE_OBJECT); }
"Texture3D"					{ return TOKENIZE(TYPE_OBJECT); }
"Texture2DMS"				{ return TOKENIZE(TYPE_OBJECT); }
"Texture2DMSArray"			{ return TOKENIZE(TYPE_OBJECT); }
"TextureCube"				{ return TOKENIZE(TYPE_OBJECT); }
"TextureCubeArray"			{ return TOKENIZE(TYPE_OBJECT); }
"RWTexture1D"				{ return TOKENIZE(TYPE_OBJECT); }
"RWTexture1DArray"			{ return TOKENIZE(TYPE_OBJECT); }
"RWTexture2D"				{ return TOKENIZE(TYPE_OBJECT); }
"RWTexture2DArray"			{ return TOKENIZE(TYPE_OBJECT); }
"RWTexture3D"				{ return TOKENIZE(TYPE_OBJECT); }

{VECTOR}		{ return TOKENIZE(TYPE_VECTOR); }
{MATRIX}		{ return TOKENIZE(TYPE_MATRIX); }
{TEXTURE}		{ return TOKENIZE(TYPE_TEXTURE); }
"SamplerState"	{ return TOKENIZE(TYPE_SAMPLER); }

POSITION[0-7]?	{ return TOKENIZE(TOKEN_SV); }
NORMAL			{ return TOKENIZE(TOKEN_SV); }
BINORMAL		{ return TOKENIZE(TOKEN_SV); }
TANGENT			{ return TOKENIZE(TOKEN_SV); }
SV_POSITION		{ return TOKENIZE(TOKEN_SV); }
SV_Position		{ return TOKENIZE(TOKEN_SV); }
SV_VertexID		{ return TOKENIZE(TOKEN_SV); }
{TEXCOORD}  	{ return TOKENIZE(TOKEN_SV); }
{Texcoord}		{ return TOKENIZE(TOKEN_SV); }
{SV_TARGET} 	{ return TOKENIZE(TOKEN_SV); }
{SV_Target}		{ return TOKENIZE(TOKEN_SV); }

"Shader"		{ return TOKENIZE(TOKEN_SHADER); }
"Properties"	{ return TOKENIZE(TOKEN_PROPERTIES); }
"Variants"		{ return TOKENIZE(TOKEN_VARIANTS); }
"Parameters"	{ return TOKENIZE(TOKEN_PARAMETERS); }
"SubShader"		{ return TOKENIZE(TOKEN_SUBSHADER); }
"Attributes"	{ return TOKENIZE(TOKEN_ATTRIBUTES); }
"MeshDrawType"	{ return TOKENIZE(TOKEN_MESHDRAWTYPE); }
"DepthTest"		{ return TOKENIZE(TOKEN_DEPTHTEST); }
"BlendMode"		{ return TOKENIZE(TOKEN_BLENDMODE); }

"using"			{ return TOKENIZE(TOKEN_USING); }
"VertexEntry"	{ return TOKENIZE(TOKEN_VERTEX); }
"PixelEntry"	{ return TOKENIZE(TOKEN_PIXEL); }

"bool"			{ return TOKENIZE(TYPE_BOOL); }
"int"       	{ return TOKENIZE(TYPE_INT); }
"uint"			{ return TOKENIZE(TYPE_UINT); }
"half"			{ return TOKENIZE(TYPE_HALF); }
"float"     	{ return TOKENIZE(TYPE_FLOAT); }
"double"		{ return TOKENIZE(TYPE_DOUBLE); }
"void"      	{ return TOKENIZE(TYPE_VOID); }

"in"			{ return TOKENIZE(TOKEN_IN); }
"inout"			{ return TOKENIZE(TOKEN_INOUT); }
"out"			{ return TOKENIZE(TOKEN_OUT); }

"struct"		{ return TOKENIZE(TOKEN_STRUCT); }

"if"			{ return TOKENIZE(TOKEN_IF); }
"else"			{ return TOKENIZE(TOKEN_ELSE); }
"true"			{ return TOKENIZE(TOKEN_TRUE); }
"false"			{ return TOKENIZE(TOKEN_FALSE); }
"for"			{ return TOKENIZE(TOKEN_FOR); }
"break"			{ return TOKENIZE(TOKEN_BREAK); }
"continue"		{ return TOKENIZE(TOKEN_CONTINUE); }
"discard"		{ return TOKENIZE(TOKEN_DISCARD); }
"return"    	{ return TOKENIZE(TOKEN_RETURN); }
"constexpr"		{ return TOKENIZE(TOKEN_CONSTEXPR); }

[a-zA-Z\_\$][a-zA-Z0-9\_\#\$]*			{ return TOKENIZE(TOKEN_IDENTIFIER);
 }
[1-9][0-9]*[uU]?						{ return TOKENIZE(TOKEN_INTEGER); }
0[xX][0-9a-fA-F]+[uU]?					{ return TOKENIZE(TOKEN_INTEGER); }
0[0-7]*[uU]?							{ return TOKENIZE(TOKEN_INTEGER); }


[0-9]+\.[0-9]+([eE][+-]?[0-9]+)?[fF]?	{ return TOKENIZE(TOKEN_FLOAT); }
\.[0-9]+([eE][+-]?[0-9]+)?[fF]?			{ return TOKENIZE(TOKEN_FLOAT); }
[0-9]+\.([eE][+-]?[0-9]+)?[fF]?			{ return TOKENIZE(TOKEN_FLOAT); }
[0-9]+[eE][+-]?[0-9]+[fF]?				{ return TOKENIZE(TOKEN_FLOAT); }
[0-9]+[fF]								{ return TOKENIZE(TOKEN_FLOAT); }

{STRINGS}		{ return TOKENIZE(STRING_CONSTANT); }

{COMMENT}		{ TOKENIZE_COMMENT(); }
{FASTCOMMENT}	{ TOKENIZE_COMMENT(); }

"+"         	{ return TOKENIZE(ADD); }
"-"         	{ return TOKENIZE(SUB); }
"*"         	{ return TOKENIZE(MUL); }
"/"         	{ return TOKENIZE(DIV); }
"="         	{ return TOKENIZE(ASSIGN); }
"%"         	{ return TOKENIZE(MOD); }

"++"        	{ return TOKENIZE(INC); }
"--"        	{ return TOKENIZE(DEC); }

"+="        	{ return TOKENIZE(ADDEQ); }
"-="        	{ return TOKENIZE(SUBEQ); }
"*="        	{ return TOKENIZE(MULEQ); }
"/="        	{ return TOKENIZE(DIVEQ); }
"%="        	{ return TOKENIZE(MODEQ); }

"<<"        	{ return TOKENIZE(LSHIFT); }
">>"        	{ return TOKENIZE(RSHIFT); }
"<<="       	{ return TOKENIZE(LSHIFTEQ); }
">>="       	{ return TOKENIZE(RSHIFTEQ); }

"&"         	{ return TOKENIZE(BITAND); }
"|"         	{ return TOKENIZE(BITOR); }
"^"         	{ return TOKENIZE(BITXOR); }
"~"         	{ return TOKENIZE(BITNOT); }
"&="        	{ return TOKENIZE(ANDEQ); }
"|="        	{ return TOKENIZE(OREQ); }
"^="        	{ return TOKENIZE(XOREQ); }

"=="        	{ return TOKENIZE(EQ); }
"!="        	{ return TOKENIZE(NE); }
"<"         	{ return TOKENIZE(LT); }
"<="        	{ return TOKENIZE(LE); }
">"         	{ return TOKENIZE(GT); }
">="        	{ return TOKENIZE(GE); }

"&&"        	{ return TOKENIZE(AND); }
"||"        	{ return TOKENIZE(OR); }
"!"         	{ return TOKENIZE(NOT); }

"?"				{ return TOKENIZE(QUESTION); }
":"         	{ return TOKENIZE(COLON); }
";"         	{ return TOKENIZE(SEMICOLON); }
","         	{ return TOKENIZE(COMMA); }
"."         	{ return TOKENIZE('.'); }
"("         	{ return TOKENIZE(LPAREN); }
")"         	{ return TOKENIZE(RPAREN); }
"{"         	{ return TOKENIZE(LBRACE); }
"}"         	{ return TOKENIZE(RBRACE); }
\[				{ return TOKENIZE(LBRACKET); }
\]				{ return TOKENIZE(RBRACKET); }


[ \t\n]     ; // Ignore blankspaces.

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
