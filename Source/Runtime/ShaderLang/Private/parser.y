
%{
#include "../Public/AstNode.h"
#include <cstdio>

using namespace Thunder;

extern FILE *yyin;
extern int yylex(void);
void yyerror(const char *);
void ThunderParse(const char* str);

ASTNode *root;
%}

%locations

%union {
    int intVal;
    char *strVal;
    struct ASTNode *astNode;
}

%token <strVal> TYPE_INT TYPE_FLOAT TYPE_VOID
%token <strVal> IDENTIFIER
%token <intVal> INT_LITERAL
%token RETURN
%token ADD SUB MUL DIV
%token ASSIGN
%token SEMICOLON COMMA
%token LPAREN RPAREN LBRACE RBRACE

%type <astNode> program function func_signature param_list param type
%type <astNode> statement_list statement var_decl assignment func_ret
%type <astNode> expression primary_expr binary_expr priority_expr

%right ASSIGN
%left ADD SUB
%left MUL DIV
%nonassoc SEMICOLON
 
%%

program:
    function {root = $1;}
    ;

function:
    func_signature LBRACE statement_list RBRACE {
        $$ = create_function_node($1, $3);
    }
    ;

func_signature:
    type IDENTIFIER LPAREN param_list RPAREN {
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
        $$ = create_param_node($1, $2);
    }
    ;

type:
    TYPE_INT { $$ = create_type_node(EVarType::TP_INT); }
    | TYPE_FLOAT { $$ = create_type_node(EVarType::TP_FLOAT); }
    | TYPE_VOID { $$ = create_type_node(EVarType::TP_VOID); }
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
    var_decl { $$ = create_statement_node($1, EStatType::Stat_DECLARE); }
    | assignment { $$ = create_statement_node($1, EStatType::Stat_ASSIGN); }
    | func_ret { $$ = create_statement_node($1, EStatType::Stat_RETURN); }
    ;

var_decl:
    type IDENTIFIER SEMICOLON {
        $$ = create_var_decl_node($1, $2, nullptr);
    }
    | type IDENTIFIER ASSIGN expression SEMICOLON {
        $$ = create_var_decl_node($1, $2, $4);
    }
    ;

assignment:
    IDENTIFIER ASSIGN expression SEMICOLON {
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
        $$ = create_binary_op_node(EBinaryOp::OP_ADD, $1, $3);
    }
    | expression SUB expression {
        $$ = create_binary_op_node(EBinaryOp::OP_SUB, $1, $3);
    }
    | expression MUL expression {
        $$ = create_binary_op_node(EBinaryOp::OP_MUL, $1, $3);
    }
    | expression DIV expression {
        $$ = create_binary_op_node(EBinaryOp::OP_DIV, $1, $3);
    }
    ;
%%

void yyerror(const char* str){
    printf("ERROR: %s\n",str);
}

void ThunderParse(const char* str)
{
    yyin = fopen(str, "r");
    if (!yyin) {
        perror("无法打开输入文件");
        return;
    }
    yyparse();
    fclose(yyin); // 关闭输入文件

    post_process_ast(root);
}