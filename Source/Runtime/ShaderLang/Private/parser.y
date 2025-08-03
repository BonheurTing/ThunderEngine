

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
%token <token> TYPE_VECTOR TYPE_MATRIX TYPE_TEXTURE TYPE_SAMPLER TYPE_OBJECT
%token <token> TYPE_BOOL TYPE_INT TYPE_UINT TYPE_HALF TYPE_FLOAT TYPE_DOUBLE TYPE_VOID
%token <token> TOKEN_IN TOKEN_INOUT TOKEN_OUT

%token <token> TOKEN_IDENTIFIER TYPE_ID NEW_ID
%token <token> TOKEN_INTEGER TOKEN_FLOAT STRING_CONSTANT

%token <token> TOKEN_SHADER TOKEN_PROPERTIES TOKEN_VARIANTS TOKEN_PARAMETERS TOKEN_SUBSHADER
%token <token> TOKEN_USING TOKEN_VERTEX TOKEN_PIXEL TOKEN_STRUCT
%token <token> TOKEN_BREAK TOKEN_CONTINUE TOKEN_DISCARD TOKEN_RETURN
%token <token> RPAREN RBRACKET RBRACE SEMICOLON
%token <token> TOKEN_IF TOKEN_ELSE TOKEN_TRUE TOKEN_FALSE TOKEN_FOR TOKEN_CONSTEXPR


/* 左结合，右结合，无结合 防止冲突 */
%nonassoc LOWER_THAN_ELSE
%nonassoc TOKEN_ELSE
%left STRING_CONSTANT
%left<%> COMMA
%right<token> QUESTION COLON
                ASSIGN ADDEQ SUBEQ MULEQ DIVEQ MODEQ LSHIFTEQ RSHIFTEQ ANDEQ OREQ XOREQ
/* 逻辑运算符优先级 */
%left<token> OR
%left<token> AND
/* 位运算符优先级 */
%left<token> BITOR
%left<token> BITXOR
%left<token> BITAND
/* 比较运算符优先级 */
%left<token> EQ NE
%left<token> LT LE GT GE
/* 移位运算符优先级 */
%left<token> LSHIFT RSHIFT
/* 算术运算符优先级 */
%left<token> ADD SUB
%left<token> MUL DIV MOD
/* 一元运算符优先级 */
%right<token> NOT BITNOT
%left<token> INC DEC
%left<token> LPAREN LBRACKET LBRACE
%left<token> '.'
%nonassoc SEMICOLON

/* %type<...> 用于指定某个非终结符语义值应该使用 %union 中的哪个字段*/
%type <token> identifier type_identifier new_identifier primary_identifier any_identifier
%type <token> primitive_types 
%type <node> definitions properties_definition variants_definition parameters_definition
%type <token> inout_modifier type_qualification
%type <type> type variable_type type_qualifications maybe_type_qualifications
%type <dimension> maybe_array array_dimensions maybe_semantic
%type <op_type> assignment_operator unary_operator
%type <node> archive_definition pass_definition stage_definition struct_definition function_definition
%type <node> program passes pass_content param_list param
%type <node> struct_members struct_member

/* statement */
%type <block> function_body block block_begin
%type <statement> statement declaration_statement jump_statement empty_statement expression_statement
                if_then_statement if_then_else_statement for_statement for_init_statement

/* expression */
%type <expression> expression primary_expr postfix_expr function_call
                  logical_or_expr logical_and_expr inclusive_or_expr exclusive_or_expr and_expr equality_expr relational_expr shift_expr
                  additive_expr multiplicative_expr unary_expr constant_expr condition maybe_condition maybe_expression
                  assignment_expr conditional_expr local_variable_initializer variable_initializer

%type <expression> function_call_header function_call_header_no_parameters function_call_header_with_parameters

%%

program:
    archive_definition { sl_state->ast_root = $1; }
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

any_identifier:
	identifier | type_identifier| new_identifier
    ;

primary_identifier:
	identifier| new_identifier
    ;

array_dimensions:
	LBRACKET RBRACKET
		{
			$$ = dimensions{};
            $$.add_dimension(0);
		}
|	LBRACKET expression RBRACKET
		{
			$$ = dimensions{};
            $$.add_dimension(sl_state->evaluate_integer_expression($2, @2));
		}
|	array_dimensions LBRACKET expression RBRACKET
		{
			$$ = $1;
            $$.add_dimension(sl_state->evaluate_integer_expression($3, @3));
		}
;

maybe_array:
		{
			$$ = dimensions{};
		}
|	array_dimensions
		{
			$$ = $1;
		}
;

maybe_semantic:
		{
			$$.set_null();
		}
| 	COLON TOKEN_SV
		{
			$$ = $2;
		}
;

archive_definition:
    TOKEN_SHADER STRING_CONSTANT LBRACE {
        sl_state->parsing_archive_begin($2);
    }
    definitions passes RBRACE {
        $$ = sl_state->parsing_archive_end($6);
    }
    ;

definitions:
    properties_definition
    | definitions variants_definition
    | definitions parameters_definition
    ;

properties_definition:
    TOKEN_PROPERTIES LBRACE
    // TODO.
    RBRACE {
        $$ = nullptr;
    }
    ;

variable:
    type new_identifier SEMICOLON {
        sl_state->add_variable_to_list($1, $2, nullptr);
    }
    | type new_identifier ASSIGN constant_expr SEMICOLON {
        sl_state->add_variable_to_list($1, $2, $4);
    }
    ;

variable_list:
    variable
    | variable_list variable
    ;

variants_definition:
    TOKEN_VARIANTS LBRACE variable_list RBRACE {
        token_data data;
        sl_state->parsing_variable_end($1, data);
        $$ = nullptr;
    }
    ;

parameters_definition:
    TOKEN_PARAMETERS STRING_CONSTANT LBRACE variable_list RBRACE {
        sl_state->parsing_variable_end($1, $2);
        $$ = nullptr;
    }
    ;

passes:
    pass_definition
    | passes pass_definition
    ;

pass_definition:
    TOKEN_SUBSHADER LBRACE {
        sl_state->parsing_pass_begin();
    }
    pass_content RBRACE{
        $$ = sl_state->parsing_pass_end();
    }
    ;

pass_content:
    stage_definition
    | struct_definition {
        sl_state->add_definition_member($1);
    }
    | function_definition {
        sl_state->add_definition_member($1);
    }
    | pass_content stage_definition
    | pass_content struct_definition {
        sl_state->add_definition_member($2);
    }
    | pass_content function_definition {
        sl_state->add_definition_member($2);
    }
    ;

stage_token:
    TOKEN_VERTEX | TOKEN_PIXEL
    ;

stage_definition:
    TOKEN_USING stage_token ASSIGN primary_identifier SEMICOLON {
        $$ = nullptr;
    }
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
        sl_state->bind_modifier($1, $4, &yylloc);
        sl_state->add_struct_member($1, $2, $4, &yylloc);  // Todo : parse sv.
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
    maybe_type_qualifications type new_identifier {
        sl_state->combine_modifier($2,$1);
        sl_state->add_function_param($2, $3, &yylloc);
    }
    ;

scalar_types:
    TYPE_BOOL | TYPE_INT | TYPE_UINT | TYPE_HALF | TYPE_FLOAT | TYPE_DOUBLE
    ;

primitive_types:
    scalar_types | TYPE_VOID | TYPE_VECTOR | TYPE_MATRIX;
    ;

type:
    primitive_types {
        $$ = sl_state->create_basic_type_node($1);
    }
    | TYPE_SAMPLER {
        $$ = sl_state->create_basic_type_node($1);
    }
    | TYPE_OBJECT {
        $$ = sl_state->create_object_type_node($1);
    }
    | TYPE_OBJECT LT primitive_types GT {
        $$ = sl_state->create_object_type_node($1, $3);
    }
    | TYPE_OBJECT LT type_identifier GT {
        $$ = sl_state->create_object_type_node($1, $3);
    }
    | type_identifier {
        $$ = sl_state->create_basic_type_node($1);
    }
    ;

inout_modifier:
	TOKEN_IN| TOKEN_INOUT| TOKEN_OUT
    ;

type_qualification:
    inout_modifier
    ;

type_qualifications:
	type_qualification
    {
        $$ = new ast_node_type_format();
        sl_state->apply_modifier($$, $1);
    }
    | type_qualifications type_qualification
    {
        sl_state->apply_modifier($$ = $1, $2);
    }
    ;

maybe_type_qualifications:
    {
        $$ = new ast_node_type_format();
    }
    | type_qualifications
    {
        $$ = $1;
    }
    ;

variable_type:
	type
    {
        $$ = $1;
    }
    | type_qualifications type
    {
        $$ = $2;
        state->combine_modifier($2, $1);
    }
    ;

local_variable_initializer:
    {
        $$ = nullptr;
    }
|	ASSIGN assignment_expr
		{
			$$ = $2;
		}
|	ASSIGN LBRACE expression RBRACE
		{
			$$ = $3; // 初始化列表，简化处理
		}
;

variable_initializer:
		{
			$$ = nullptr;
		}
|	ASSIGN constant_expr
		{
			$$ = $2;
		}
|	ASSIGN LBRACE constant_expr RBRACE
		{
			$$ = $3; // 常量初始化列表
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
    block { $$ = $1; }
    | declaration_statement
    | jump_statement
    | empty_statement
    | expression_statement
    | if_then_statement
    | if_then_else_statement
    | for_statement;

declaration_statement:
    variable_type primary_identifier maybe_array local_variable_initializer SEMICOLON
    {
        $$ = sl_state->create_declaration_statement($1, $2, $3, $4);
    }
    ;

jump_statement:
    TOKEN_RETURN expression SEMICOLON {
        $$ = sl_state->create_return_statement($2);
    }
    | TOKEN_RETURN SEMICOLON {
        $$ = sl_state->create_return_statement(nullptr);
    }
    | TOKEN_BREAK SEMICOLON {
        $$ = sl_state->create_break_statement();
    }
    | TOKEN_CONTINUE SEMICOLON {
        $$ = sl_state->create_continue_statement();
    }
    | TOKEN_DISCARD SEMICOLON {
        $$ = sl_state->create_discard_statement();
    }
    ;

empty_statement:
    SEMICOLON
    {
        $$ = nullptr;
    }
    ;

expression_statement:
	expression SEMICOLON
    {
        $$ = sl_state->create_expression_statement($1);
    }
    ;

if_head:
    TOKEN_IF
    | TOKEN_IF TOKEN_CONSTEXPR;

if_then_statement:
	if_head LPAREN expression RPAREN statement %prec LOWER_THAN_ELSE
    {
        $$ = sl_state->create_condition_statement($3, $5, nullptr);
    }
;

if_then_else_statement:
    if_head LPAREN expression RPAREN statement TOKEN_ELSE statement
    {
        $$ = sl_state->create_condition_statement($3, $5, $7);
    }
    ;

for_statement:
	TOKEN_FOR LPAREN for_init_statement maybe_condition SEMICOLON maybe_expression RPAREN statement
    {
        $$ = sl_state->create_for_statement($3, $4, $6, $8);
    }
    ;

for_init_statement:
	expression_statement
    | declaration_statement
    | empty_statement
    ;

condition:
	expression
    {
        $$ = $1;
    }
    | type any_identifier ASSIGN assignment_expr
    {
        $$ = $4;
    }
    ;

maybe_condition:
    {
        $$ = nullptr;
    }
	| condition
    ;

maybe_expression:
    {
        $$ = nullptr;
    }
    | expression
    ;

expression:
    assignment_expr
    | expression COMMA assignment_expr
    {
        $$ = sl_state->create_chain_expression($1, $3);
    }
    ;

assignment_expr:
    conditional_expr
    | unary_expr assignment_operator assignment_expr
    {
        $$ = sl_state->create_compound_assignment_expression($2, $1, $3);
    }
    ;

conditional_expr:
    logical_or_expr
    | logical_or_expr QUESTION expression COLON assignment_expr
    {
        $$ = sl_state->create_conditional_expression($1, $3, $5);
    }
    ;

/* 逻辑或表达式 */
logical_or_expr:
    logical_and_expr
    {
        $$ = $1;
    }
    | logical_or_expr OR logical_and_expr
    {
        $$ = sl_state->create_binary_expression(enum_binary_op::logical_or, $1, $3);
    }
    ;

/* 逻辑与表达式 */
logical_and_expr:
    inclusive_or_expr
    {
        $$ = $1;
    }
    | logical_and_expr AND inclusive_or_expr
    {
        $$ = sl_state->create_binary_expression(enum_binary_op::logical_and, $1, $3);
    }
    ;

/* 位或表达式 */
inclusive_or_expr:
    exclusive_or_expr
    {
        $$ = $1;
    }
    | inclusive_or_expr BITOR exclusive_or_expr
    {
        $$ = sl_state->create_binary_expression(enum_binary_op::bit_or, $1, $3);
    }
    ;

/* 位异或表达式 */
exclusive_or_expr:
    and_expr
    {
        $$ = $1;
    }
    | exclusive_or_expr BITXOR and_expr
    {
        $$ = sl_state->create_binary_expression(enum_binary_op::bit_xor, $1, $3);
    }
    ;

/* 位与表达式 */
and_expr:
    equality_expr
    {
        $$ = $1;
    }
    | and_expr BITAND equality_expr
    {
        $$ = sl_state->create_binary_expression(enum_binary_op::bit_and, $1, $3);
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
        $$ = sl_state->create_binary_expression(enum_binary_op::equal, $1, $3);
    }
    | equality_expr NE relational_expr
    {
        $$ = sl_state->create_binary_expression(enum_binary_op::not_equal, $1, $3);
    }
    ;

/* 关系表达式 */
relational_expr:
    shift_expr
    {
        $$ = $1;
    }
    | relational_expr LT shift_expr
    {
        $$ = sl_state->create_binary_expression(enum_binary_op::less, $1, $3);
    }
    | relational_expr LE shift_expr
    {
        $$ = sl_state->create_binary_expression(enum_binary_op::less_equal, $1, $3);
    }
    | relational_expr GT shift_expr
    {
        $$ = sl_state->create_binary_expression(enum_binary_op::greater, $1, $3);
    }
    | relational_expr GE shift_expr
    {
        $$ = sl_state->create_binary_expression(enum_binary_op::greater_equal, $1, $3);
    }
    ;

/* 移位表达式 */
shift_expr:
    additive_expr
    {
        $$ = $1;
    }
    | shift_expr LSHIFT additive_expr
    {
        $$ = sl_state->create_binary_expression(enum_binary_op::left_shift, $1, $3);
    }
    | shift_expr RSHIFT additive_expr
    {
        $$ = sl_state->create_binary_expression(enum_binary_op::right_shift, $1, $3);
    }
    ;

/* 加法表达式 */
additive_expr:
    multiplicative_expr
    {
        $$ = $1;
    }
    | additive_expr ADD multiplicative_expr
    {
        $$ = sl_state->create_binary_expression(enum_binary_op::add, $1, $3);
    }
    | additive_expr SUB multiplicative_expr
    {
        $$ = sl_state->create_binary_expression(enum_binary_op::sub, $1, $3);
    }
    ;

/* 乘法表达式 */
multiplicative_expr:
    unary_expr
    {
        $$ = $1;
    }
    | multiplicative_expr MUL unary_expr
    {
        $$ = sl_state->create_binary_expression(enum_binary_op::mul, $1, $3);
    }
    | multiplicative_expr DIV unary_expr
    {
        $$ = sl_state->create_binary_expression(enum_binary_op::div, $1, $3);
    }
    | multiplicative_expr MOD unary_expr
    {
        $$ = sl_state->create_binary_expression(enum_binary_op::mod, $1, $3);
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

assignment_operator:
    ASSIGN
    {
        $$ = static_cast<int>(enum_assignment_op::assign);
    }
    | ADDEQ
    {
        $$ = static_cast<int>(enum_assignment_op::add_assign);
    }
    | SUBEQ
    {
        $$ = static_cast<int>(enum_assignment_op::sub_assign);
    }
    | MULEQ
    {
        $$ = static_cast<int>(enum_assignment_op::mul_assign);
    }
    | DIVEQ
    {
        $$ = static_cast<int>(enum_assignment_op::div_assign);
    }
    | MODEQ
    {
        $$ = static_cast<int>(enum_assignment_op::mod_assign);
    }
    | LSHIFTEQ
    {
        $$ = static_cast<int>(enum_assignment_op::lshift_assign);
    }
    | RSHIFTEQ
    {
        $$ = static_cast<int>(enum_assignment_op::rshift_assign);
    }
    | ANDEQ
    {
        $$ = static_cast<int>(enum_assignment_op::and_assign);
    }
    | OREQ
    {
        $$ = static_cast<int>(enum_assignment_op::or_assign);
    }
    | XOREQ
    {
        $$ = static_cast<int>(enum_assignment_op::xor_assign);
    }
    ;
    
unary_operator:
    ADD
    {
        $$ = static_cast<int>(enum_unary_op::positive);
    }
    | SUB
    {
        $$ = static_cast<int>(enum_unary_op::negative);
    }
    | NOT
    {
        $$ = static_cast<int>(enum_unary_op::logical_not);
    }
    | BITNOT
    {
        $$ = static_cast<int>(enum_unary_op::bit_not);
    }
    | INC
    {
        $$ = static_cast<int>(enum_unary_op::pre_inc);
    }
    | DEC
    {
        $$ = static_cast<int>(enum_unary_op::pre_dec);
    }
    ;

/* 一元表达式 */
unary_expr:
    postfix_expr
    {
        $$ = $1;
    }
    | unary_operator unary_expr
    {
        $$ = sl_state->create_unary_expression($1, $2);
    }
    | LPAREN type RPAREN unary_expr
    {
        $$ = sl_state->create_cast_expression($2, $4);
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
    | postfix_expr INC
    {
        $$ = sl_state->create_unary_expression(static_cast<int>(enum_unary_op::post_inc), $1);
    }
    | postfix_expr DEC
    {
        $$ = sl_state->create_unary_expression(static_cast<int>(enum_unary_op::post_dec), $1);
    }
    | postfix_expr LBRACKET expression RBRACKET
    {
        $$ = sl_state->create_index_expression($1, $3);
    }
    | function_call
    {
		$$ = $1;
    }
    ;

function_call_header:
    type LPAREN
    {
        $$ = sl_state->create_constructor_expression($1);
    }
    | primary_identifier LPAREN
    {
        $$ = sl_state->create_function_call_expression($1);
    }
    ;

function_call_header_no_parameters:
    function_call_header TYPE_VOID
    | function_call_header
    ;

function_call_header_with_parameters:
    function_call_header assignment_expr
    {
        $$ = $1;
		state->append_argument($$, $2);
    }
    | function_call_header_with_parameters COMMA assignment_expr
    {
        $$ = $1;
		state->append_argument($$, $3);
    }
    ;

function_call:
    function_call_header_no_parameters RPAREN
    | function_call_header_with_parameters RPAREN
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