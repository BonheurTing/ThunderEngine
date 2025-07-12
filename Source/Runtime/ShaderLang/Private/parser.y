

%{
#include "../Public/AstNode.h"
#include "../Public/ShaderLang.h"
#include <cstdio>

using namespace Thunder;
void yyerror(parse_location *loc, shader_lang_state* st, const char* msg);

extern void lexer_constructor(struct shader_lang_state *state, const char* text);
extern void lexer_lexer_dtor(struct shader_lang_state *state);
extern int yyparse(struct shader_lang_state *state);

void ThunderParse(const char* text);

#define YYLEX_PARAM sl_state->scanner
#define YYLTYPE parse_location
#define YYSTYPE parse_node
#define YYLLOC_DEFAULT(Current, Rhs, N)                                \
	do                                                                 \
	  if (N)                                                           \
		{                                                              \
		  int i = 1, j = N;                                            \
		  while(YYRHSLOC(Rhs, i).is_null() && i < N) i += 1;           \
		  while(YYRHSLOC(Rhs, j).is_null() && j > i) j -= 1;           \
		  (Current).first_line   = YYRHSLOC(Rhs, i).first_line;        \
		  (Current).first_column = YYRHSLOC(Rhs, i).first_column;      \
		  (Current).first_source = YYRHSLOC(Rhs, i).first_source;      \
		  (Current).last_line    = YYRHSLOC(Rhs, j).last_line;         \
		  (Current).last_column  = YYRHSLOC(Rhs, j).last_column;       \
		  (Current).last_source  = YYRHSLOC(Rhs, j).last_source;       \
		}                                                              \
	  else                                                             \
		{                                                              \
		  (Current).set_null();                                        \
		}                                                              \
	while (0)


int yylex(YYSTYPE *, parse_location*, void*);

#define YYDEBUG 1

%}

%define api.pure full

%locations
%initial-action {
   @$.first_line = 1;
   @$.first_column = 1;
   @$.first_source = 0;
   @$.last_line = 1;
   @$.last_column = 1;
   @$.last_source = 0;
}

%lex-param			{ void* sl_state->scanner }
%parse-param		{ struct shader_lang_state *state }

/* %token 用于声明 终结符（terminal/token），即词法分析器（lexer/flex）返回的词法单元。
它可以指定该 token 的语义值类型（通过 <...> 指定 %union 的成员） */
%token <token> TOKEN_SV
%token <token> TYPE_VECTOR TYPE_MATRIX TYPE_TEXTURE TYPE_SAMPLER
%token <token> TOKEN_SHADER TOKEN_PROPERTIES TOKEN_SUBSHADER TOKEN_RETURN TOKEN_STRUCT
%token <token> TYPE_INT TYPE_FLOAT TYPE_VOID
%token <token> TOKEN_IDENTIFIER TYPE_ID NEW_ID
%token <token> TOKEN_INTEGER STRING_CONSTANT
%token <token> SEMICOLON
%token <token> RPAREN RBRACE

/* 左结合，右结合，无结合 防止冲突 */
%left STRING_CONSTANT
%left<%> COMMA
%right<token> ASSIGN COLON
%left<token> ADD SUB
%left<token> MUL DIV
%left		 LPAREN LBRACE
%left<token> '.'
%nonassoc SEMICOLON

/* %type<...> 用于指定某个非终结符语义值应该使用 %union 中的哪个字段*/
%type <token> identifier type_identifier new_identifier primary_identifier
%type <token> primitive_types 
%type properties_definition /* Test. */
%type <node> type
%type <node> arrchive_definition pass_definition stage_definition struct_definition function_definition
%type <node> program passes pass_content param_list param
%type <node> struct_members struct_member
%type <node> function_body statement_list statement var_decl assignment func_ret
%type <expression> expression primary_expr binary_expr postfix_expr

 
%%

program:
    arrchive_definition { sl_state->ast_root = $1; }
    ;

identifier:
    TOKEN_IDENTIFIER
    ;

type_identifier:
	TYPE_ID
    ;

new_identifier:
	NEW_ID
    ;

/*
any_identifier:
	identifier | type_identifier| new_identifier
; */

primary_identifier:
	identifier| new_identifier
    ;

arrchive_definition:
    TOKEN_SHADER STRING_CONSTANT LBRACE properties_definition passes RBRACE {
        $$ = $5;
    }
    ;

properties_definition:
    TOKEN_PROPERTIES LBRACE
    // TODO.
    RBRACE
    ;

passes:
    pass_definition
    | passes pass_definition
    ;

pass_definition:
    TOKEN_SUBSHADER LBRACE pass_content RBRACE{
        $$ = $3;
    }
    ;

pass_content:
    struct_definition stage_definition{
        $$ = create_pass_node($1, $2);
    }
    ;

stage_definition:
    function_definition
    ;

struct_definition:
    TOKEN_STRUCT new_identifier LBRACE {
        sl_state->parsing_struct_begin($2);
    }
    struct_members RBRACE SEMICOLON {
        $$ = sl_state->parsing_struct_end();
    }
    ;

struct_members:
    struct_member
    | struct_members struct_member
    ;

struct_member:
    type new_identifier SEMICOLON {
        token_data token;
        sl_state->add_struct_member($1, $2, token, &yylloc);
    }
    | type new_identifier COLON TOKEN_SV SEMICOLON {
        ast_node* type = $1;
        sl_state->bind_modifier(type, $4, &yylloc);
        sl_state->add_struct_member(type, $2, $4, &yylloc);  // Todo : parse sv.
    }
    ;

function_definition:
    type new_identifier LPAREN {
        sl_state->parsing_function_begin($1, $2);
    }
    param_list RPAREN LBRACE function_body RBRACE {
        $$ = sl_state->parsing_function_end();
    }
    ;

param_list:
    // Empty.
    {
    }
    | param_list COMMA param
    | param
    ;

param:
    type new_identifier {
        sl_state->add_function_param($1, $2, &yylloc);
    }
    ;

primitive_types:
    identifier
	| TYPE_INT| TYPE_FLOAT| TYPE_VOID 
    | TYPE_VECTOR | TYPE_MATRIX | TYPE_TEXTURE | TYPE_SAMPLER
    ;

type:
    primitive_types {
        $$ = create_type_node($1);
    }
    | type_identifier {
        $$ = create_type_node($1);
    }
    ;

function_body:
    statement_list {
        sl_state->set_function_body($1, &yylloc);
    }
    ;

statement_list:
    // Empty.
    {
        $$ = create_statement_list_node();
    }
    | statement_list statement {
        add_statement_to_list($1, $2);
        $$ = $1;
    }
    ;

statement:
    var_decl | assignment | func_ret ;

var_decl:
    type new_identifier SEMICOLON {
        $$ = create_var_decl_node($1, $2, nullptr);
    }
    | type new_identifier ASSIGN expression SEMICOLON {
        $$ = create_var_decl_node($1, $2, $4);
    }
    ;

assignment:
    identifier ASSIGN expression SEMICOLON {
        sl_state->evaluate_symbol($1, enum_symbol_type::variable);
        $$ = create_assignment_node($1, $3);
    }
    ;

func_ret:
    TOKEN_RETURN expression SEMICOLON {
        $$ = create_return_node($2);
    }
    ;

expression:
    primary_expr | binary_expr | postfix_expr;

primary_expr:
    primary_identifier
    {
        $$ = sl_state->create_reference_expression($1);
    }
    |
    LPAREN expression RPAREN
    {
        $$ = $2;
    }
    ;

binary_expr:
    expression ADD expression {
        $$ = sl_state->create_binary_op_expression(enum_binary_op::add, $1, $3);
    }
    | expression SUB expression {
        $$ = sl_state->create_binary_op_expression(enum_binary_op::sub, $1, $3);
    }
    | expression MUL expression {
        $$ = sl_state->create_binary_op_expression(enum_binary_op::mul, $1, $3);
    }
    | expression DIV expression {
        $$ = sl_state->create_binary_op_expression(enum_binary_op::div, $1, $3);
    }
    ;

postfix_expr:
    primary_expr
    {
        $$ = $1;
    }
    | postfix_expr '.' primary_identifier
    {
        $$ = sl_state->create_shuffle_or_component_expression($1, $3);
    }
    ;

%%


void yyerror(parse_location *loc, shader_lang_state* st, const char* msg){
    printf("ERROR: %s\n",msg);
}

void ThunderParse(const char* text)
{
    yydebug = 1;
    sl_state = new shader_lang_state();
    lexer_constructor(sl_state, text);
    yyparse(sl_state);
    post_process_ast(sl_state->ast_root);
	lexer_lexer_dtor(sl_state);
}