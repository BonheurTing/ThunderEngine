

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
%token <token> TYPE_INT TYPE_FLOAT TYPE_VOID

%token <token> TOKEN_IDENTIFIER TYPE_ID NEW_ID
%token <token> TOKEN_INTEGER TOKEN_FLOAT STRING_CONSTANT

%token <token> TOKEN_SHADER TOKEN_PROPERTIES TOKEN_SUBSHADER TOKEN_RETURN TOKEN_STRUCT
%token <token> RPAREN RBRACE SEMICOLON
%token <token> TOKEN_IF TOKEN_ELSE TOKEN_TRUE TOKEN_FALSE

/* 逻辑和比较运算符 */
%token <token> EQ NE LT LE GT GE AND OR NOT

/* 左结合，右结合，无结合 防止冲突 */
%nonassoc LOWER_THAN_ELSE
%nonassoc TOKEN_ELSE
%left STRING_CONSTANT
%left<%> COMMA
%right<token> ASSIGN COLON
/* 逻辑运算符优先级 */
%left OR
%left AND
/* 比较运算符优先级 */
%left EQ NE
%left LT LE GT GE
/* 算术运算符优先级 */
%left ADD SUB
%left MUL DIV
/* 一元运算符优先级 */
%right NOT
%left		 LPAREN LBRACE
%left<token> '.'
%nonassoc SEMICOLON

/* %type<...> 用于指定某个非终结符语义值应该使用 %union 中的哪个字段*/
%type <token> identifier type_identifier new_identifier primary_identifier
%type <token> primitive_types 
%type properties_definition /* Test. */
%type <type> type
%type <node> arrchive_definition pass_definition stage_definition struct_definition function_definition
%type <node> program passes pass_content param_list param
%type <node> struct_members struct_member

/* statement */
%type <block> function_body block block_begin
%type <statement> statement var_decl assignment func_ret empty_statement 
                if_then_statement if_then_else_statement

/* expression */
%type <expression> expression primary_expr binary_expr postfix_expr
                  logical_or_expr logical_and_expr equality_expr relational_expr
                  unary_expr constant_expr
 
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
        $$ = sl_state->create_pass_node($1, $2);
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
    param_list RPAREN function_body {
        $$ = sl_state->parsing_function_end($7);
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
        $$ = sl_state->create_type_node($1);
    }
    | type_identifier {
        $$ = sl_state->create_type_node($1);
    }
    ;

function_body:
    block
    ;

block_begin:
	LBRACE
    {
        sl_state->parsing_block_begin();
    }
    ;

block_end:
	RBRACE
    ;

block:
    block_begin block_end
    {
        $$ = sl_state->parsing_block_end();
    }
    | block_begin block_statements block_end
    {
        $$ = sl_state->parsing_block_end();
    }
    ;

block_statements:
    statement 
    {
        sl_state->add_block_statement($1, &yylloc);
    }
    | block_statements statement
    {
        sl_state->add_block_statement($2, &yylloc);
    }

statement:
    var_decl | assignment | func_ret | empty_statement
    | if_then_statement | if_then_else_statement;

var_decl:
    type new_identifier SEMICOLON {
        $$ = sl_state->create_var_decl_statement($1, $2, nullptr);
    }
    | type new_identifier ASSIGN expression SEMICOLON {
        $$ = sl_state->create_var_decl_statement($1, $2, $4);
    }
    ;

assignment:
    identifier ASSIGN expression SEMICOLON {
        sl_state->evaluate_symbol($1, enum_symbol_type::variable);
        $$ = sl_state->create_assignment_statement($1, $3);
    }
    ;

func_ret:
    TOKEN_RETURN expression SEMICOLON {
        $$ = sl_state->create_return_statement($2);
    }
    ;

empty_statement:
    SEMICOLON
    {
        $$ = nullptr;
    }
    ;

if_then_statement:
	TOKEN_IF LPAREN expression RPAREN statement %prec LOWER_THAN_ELSE
    {
        $$ = state->create_condition_statement($3, $5, nullptr);
    }
;

if_then_else_statement:
    TOKEN_IF LPAREN expression RPAREN statement TOKEN_ELSE statement
    {
        $$ = state->create_condition_statement($3, $5, $7);
    }
    ;


expression:
    logical_or_expr;

/* 逻辑或表达式 */
logical_or_expr:
    logical_and_expr
    {
        $$ = $1;
    }
    | logical_or_expr OR logical_and_expr
    {
        $$ = sl_state->create_binary_op_expression(enum_binary_op::logical_or, $1, $3);
    }
    ;

/* 逻辑与表达式 */
logical_and_expr:
    equality_expr
    {
        $$ = $1;
    }
    | logical_and_expr AND equality_expr
    {
        $$ = sl_state->create_binary_op_expression(enum_binary_op::logical_and, $1, $3);
    }
    ;

/* 相等性表达式 */
equality_expr:
    relational_expr
    {
        $$ = $1;
    }
    | equality_expr EQ relational_expr
    {
        $$ = sl_state->create_binary_op_expression(enum_binary_op::equal, $1, $3);
    }
    | equality_expr NE relational_expr
    {
        $$ = sl_state->create_binary_op_expression(enum_binary_op::not_equal, $1, $3);
    }
    ;

/* 关系表达式 */
relational_expr:
    binary_expr
    {
        $$ = $1;
    }
    | relational_expr LT binary_expr
    {
        $$ = sl_state->create_binary_op_expression(enum_binary_op::less, $1, $3);
    }
    | relational_expr LE binary_expr
    {
        $$ = sl_state->create_binary_op_expression(enum_binary_op::less_equal, $1, $3);
    }
    | relational_expr GT binary_expr
    {
        $$ = sl_state->create_binary_op_expression(enum_binary_op::greater, $1, $3);
    }
    | relational_expr GE binary_expr
    {
        $$ = sl_state->create_binary_op_expression(enum_binary_op::greater_equal, $1, $3);
    }
    ;

primary_expr:
    primary_identifier
    {
        $$ = sl_state->create_reference_expression($1);
    }
    | constant_expr
    {
        $$ = $1;
    }
    |
    LPAREN expression RPAREN
    {
        $$ = $2;
    }
    ;

/* 常量表达式 */
constant_expr:
    TOKEN_INTEGER
    {
        $$ = new constant_int_expression(std::stoi($1.text));
    }
    | TOKEN_FLOAT
    {
        $$ = new constant_float_expression(std::stof($1.text));
    }
    | TOKEN_TRUE
    {
        $$ = new constant_bool_expression(true);
    }
    | TOKEN_FALSE
    {
        $$ = new constant_bool_expression(false);
    }
    ;

/* 一元表达式 */
unary_expr:
    postfix_expr
    {
        $$ = $1;
    }
    | NOT unary_expr
    {
        // 需要在AstNode.h中定义unary_expression类
        // $$ = new unary_expression(enum_unary_op::logical_not, $2);
        $$ = $2; // 临时实现
    }
    ;

binary_expr:
    unary_expr
    {
        $$ = $1;
    }
    | binary_expr ADD binary_expr {
        $$ = sl_state->create_binary_op_expression(enum_binary_op::add, $1, $3);
    }
    | binary_expr SUB binary_expr {
        $$ = sl_state->create_binary_op_expression(enum_binary_op::sub, $1, $3);
    }
    | binary_expr MUL binary_expr {
        $$ = sl_state->create_binary_op_expression(enum_binary_op::mul, $1, $3);
    }
    | binary_expr DIV binary_expr {
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
    sl_state->post_process_ast();
	lexer_lexer_dtor(sl_state);
}