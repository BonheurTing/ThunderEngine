

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
%token TOKEN_SHADER TOKEN_PROPERTIES TOKEN_SUBSHADER TOKEN_RETURN TOKEN_STRUCT
%token TYPE_INT TYPE_FLOAT TYPE_VOID
%token <str_val> IDENTIFIER
%token <int_val> INT_LITERAL
%token ADD SUB MUL DIV
%token ASSIGN COLON SEMICOLON COMMA QUOTE 
%token LPAREN RPAREN LBRACE RBRACE

/* %type<...> 用于指定某个非终结符语义值应该使用 %union 中的哪个字段*/
%type <token> primitive_types
%type <node> type
%type <node> arrchive_definition properties_definition pass_definition stage_definition struct_definition function_definition
%type <node> program passes pass_content function_signature param_list param
%type <node> struct_members struct_member
%type <node> statement_list statement var_decl assignment func_ret
%type <node> expression primary_expr binary_expr priority_expr

%right ASSIGN
%left ADD SUB
%left MUL DIV
%nonassoc SEMICOLON
 
%%

program:
    arrchive_definition {sl_state->ast_root = $1;}
    ;

arrchive_definition:
    TOKEN_SHADER QUOTE IDENTIFIER QUOTE LBRACE properties_definition passes RBRACE {
        $$ = $7;
    }
    ;

properties_definition:
    TOKEN_PROPERTIES LBRACE /*TODO*/ RBRACE
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
    function_definition{
        $$ = $1;
    }
    ;

struct_definition:
    TOKEN_STRUCT IDENTIFIER LBRACE {
        sl_state->parsing_struct_begin($2, &yylloc);
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
    type IDENTIFIER SEMICOLON {
        token_data token;
        sl_state->add_struct_member($1, $2, token, &yylloc);
    }
    | type IDENTIFIER COLON TOKEN_SV SEMICOLON {
        ast_node* type = $1;
        sl_state->bind_modifier(type, $4, &yylloc);
        sl_state->add_struct_member(type, $2, $4, &yylloc);  /* todo: parse sv */
    }
    ;

function_definition:
    function_signature LBRACE statement_list RBRACE {
        $$ = create_function_node($1, $3);
    }
    ;

function_signature:
    type IDENTIFIER LPAREN param_list RPAREN {
        sl_state->insert_symbol_table($2, enum_symbol_type::function, &yylloc);
        $$ = create_func_signature_node($1, $2, $4);
    }
    ;

param_list:
    /* empty */ {
        $$ = create_param_list_node();
    }
    | param_list COMMA param {
        add_param_to_list($1, $3);
        $$ = $1;
    }
    | param {
        $$ = create_param_list_node();
        add_param_to_list($$, $1);
    }
    ;

param:
    type IDENTIFIER {
        sl_state->insert_symbol_table($2, enum_symbol_type::variable, &yylloc);
        $$ = create_param_node($1, $2);
    }
    ;

primitive_types:
	TYPE_INT| TYPE_FLOAT| TYPE_VOID 
    | TYPE_VECTOR | TYPE_MATRIX | TYPE_TEXTURE | TYPE_SAMPLER
    ;

type:
    primitive_types {
        $$ = create_type_node($1);
    }
    ;

statement_list:
    /* empty */ {
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
    type IDENTIFIER SEMICOLON {
        sl_state->insert_symbol_table($2, enum_symbol_type::variable, &yylloc);
        $$ = create_var_decl_node($1, $2, nullptr);
    }
    | type IDENTIFIER ASSIGN expression SEMICOLON {
        sl_state->insert_symbol_table($2, enum_symbol_type::variable, &yylloc);
        $$ = create_var_decl_node($1, $2, $4);
    }
    ;

assignment:
    IDENTIFIER ASSIGN expression SEMICOLON {
        sl_state->evaluate_symbol($1, enum_symbol_type::variable, &yylloc);
        $$ = create_assignment_node($1, $3);
    }
    ;

func_ret:
    TOKEN_RETURN expression SEMICOLON {
        $$ = create_return_node($2);
    }
    ;

expression:
    primary_expr | priority_expr | binary_expr ;

primary_expr:
    INT_LITERAL { $$ = create_int_literal_node($1); }
    | IDENTIFIER { $$ = create_identifier_node($1); }
    ;

priority_expr:
    LPAREN /* empty */ RPAREN {$$ = create_priority_node(nullptr); }
    | LPAREN expression RPAREN { $$ = create_priority_node($2); }
    ;

binary_expr:
    expression ADD expression {
        $$ = create_binary_op_node(enum_binary_op::add, $1, $3);
    }
    | expression SUB expression {
        $$ = create_binary_op_node(enum_binary_op::sub, $1, $3);
    }
    | expression MUL expression {
        $$ = create_binary_op_node(enum_binary_op::mul, $1, $3);
    }
    | expression DIV expression {
        $$ = create_binary_op_node(enum_binary_op::div, $1, $3);
    }
    ;
%%


void yyerror(parse_location *loc, shader_lang_state* st, const char* msg){
    printf("ERROR: %s\n",msg);
}

void ThunderParse(const char* text)
{
    sl_state = new shader_lang_state();
    lexer_constructor(sl_state, text);
    yyparse(sl_state);
    post_process_ast(sl_state->ast_root);
	lexer_lexer_dtor(sl_state);
}