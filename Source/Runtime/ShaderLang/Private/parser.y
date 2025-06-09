
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

shader_lang_state *sl_state;

#define YYLEX_PARAM sl_state->scanner
#define YYLTYPE parse_location
#define YYSTYPE parse_node


int yylex(YYSTYPE *, parse_location*, void*);

%}

%locations

%define api.pure
%lex-param			{ void* sl_state->scanner }
%parse-param		{ struct shader_lang_state *state }

%token TYPE_INT TYPE_FLOAT TYPE_VOID
%token <str_val> IDENTIFIER
%token <int_val> INT_LITERAL
%token RETURN
%token ADD SUB MUL DIV
%token ASSIGN
%token SEMICOLON COMMA
%token LPAREN RPAREN LBRACE RBRACE

%type <token> program function func_signature param_list param type
%type <token> statement_list statement var_decl assignment func_ret
%type <token> expression primary_expr binary_expr priority_expr

%right ASSIGN
%left ADD SUB
%left MUL DIV
%nonassoc SEMICOLON
 
%%

program:
    function {sl_state->ast_root = $1;}
    ;

function:
    func_signature LBRACE statement_list RBRACE {
        $$ = create_function_node($1, $3);
    }
    ;

func_signature:
    type IDENTIFIER LPAREN param_list RPAREN {
        sl_state->insert_symbol_table($2, enum_symbol_type::function);
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
        sl_state->insert_symbol_table($2, enum_symbol_type::variable);
        $$ = create_param_node($1, $2);
    }
    ;

type:
    TYPE_INT { $$ = create_type_node(var_type::tp_int); }
    | TYPE_FLOAT { $$ = create_type_node(var_type::tp_float); }
    | TYPE_VOID { $$ = create_type_node(var_type::tp_void); }
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
    var_decl { $$ = $1; }
    | assignment { $$ = $1; }
    | func_ret { $$ = $1; }
    ;

var_decl:
    type IDENTIFIER SEMICOLON {
        sl_state->insert_symbol_table($2, enum_symbol_type::variable);
        $$ = create_var_decl_node($1, $2, nullptr);
    }
    | type IDENTIFIER ASSIGN expression SEMICOLON {
        sl_state->insert_symbol_table($2, enum_symbol_type::variable);
        $$ = create_var_decl_node($1, $2, $4);
    }
    ;

assignment:
    IDENTIFIER ASSIGN expression SEMICOLON {
        sl_state->validate_symbol($1, enum_symbol_type::variable);
        $$ = create_assignment_node($1, $3);
    }
    ;

func_ret:
    RETURN expression SEMICOLON {
        $$ = create_return_node($2);
    }
    ;

expression:
    primary_expr { $$ = $1; }
    | priority_expr { $$ = $1; }
    | binary_expr { $$ = $1; }
    ;

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
        $$ = create_binary_op_node(binary_op::add, $1, $3);
    }
    | expression SUB expression {
        $$ = create_binary_op_node(binary_op::sub, $1, $3);
    }
    | expression MUL expression {
        $$ = create_binary_op_node(binary_op::mul, $1, $3);
    }
    | expression DIV expression {
        $$ = create_binary_op_node(binary_op::div, $1, $3);
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