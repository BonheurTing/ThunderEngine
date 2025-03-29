%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
// 符号表：存储变量名和值
#define MAX_VARS 100
typedef struct {
    char *name;
    int value;
} Variable;
 
Variable symbol_table[MAX_VARS];
int symbol_count = 0;
 
void set_variable_value(const char *name, int value) {
    // 检查符号表中是否已有该变量
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            symbol_table[i].value = value;
            return;
        }
    }
    // 如果没有，插入新变量
    symbol_table[symbol_count].name = strdup(name);
    symbol_table[symbol_count].value = value;
    symbol_count++;
}
 
int get_variable_value(const char *name) {
    // 查找变量值
    for (int i = 0; i < symbol_count; i++) {
        if (strcmp(symbol_table[i].name, name) == 0) {
            return symbol_table[i].value;
        }
    }
    // 如果未找到变量，报错并退出
    printf("Error: Undefined variable %s\n", name);
    exit(1);
}
 
int yylex(void);
void yyerror(char *);
 
// 定义抽象语法树节点
typedef enum {
    NODE_NUMBER,
    NODE_IDENTIFIER,
    NODE_ADD,
    NODE_SUB,
    NODE_MUL,
    NODE_DIV,
    NODE_ASSIGN,
    NODE_PAREN
} NodeType;
 
typedef struct ASTNode {
    NodeType type;
    int value; // 用于存储数字
    char *name; // 用于存储变量名
    struct ASTNode *left;
    struct ASTNode *right;
} ASTNode;
 
ASTNode* createNode(NodeType type, int value, char *name, ASTNode *left, ASTNode *right) {
    ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = type;
    node->value = value;
    node->name = name;
    node->left = left;
    node->right = right;
    return node;
}
 
int evaluateNode(ASTNode *node) {
    if (node == NULL) return 0;
    switch (node->type) {
        case NODE_NUMBER:
            return node->value;
        case NODE_IDENTIFIER:
            return get_variable_value(node->name);
        case NODE_ADD:
            return evaluateNode(node->left) + evaluateNode(node->right);
        case NODE_SUB:
            return evaluateNode(node->left) - evaluateNode(node->right);
        case NODE_MUL:
            return evaluateNode(node->left) * evaluateNode(node->right);
        case NODE_DIV:
            return evaluateNode(node->left) / evaluateNode(node->right);
        case NODE_ASSIGN:
            return node->right->value;
        case NODE_PAREN:
            return evaluateNode(node->left);
        default:
            return 0;
    }
}
 
void printAST(ASTNode *node, int level) {
    if (node == NULL) return;
    for (int i = 0; i < level; i++) printf("  ");
    switch (node->type) {
        case NODE_NUMBER:
            printf("Number: %d\n", node->value);
            break;
        case NODE_IDENTIFIER:
            printf("Identifier: %s\n", node->name);
            break;
        case NODE_ADD:
            printf("ADD\n");
            break;
        case NODE_SUB:
            printf("SUB\n");
            break;
        case NODE_MUL:
            printf("MUL\n");
            break;
        case NODE_DIV:
            printf("DIV\n");
            break;
        case NODE_ASSIGN:
            printf("Assign\n");
            break;
        case NODE_PAREN:
            printf("Paren\n");
            break;
    }
    printAST(node->left, level + 1);
    printAST(node->right, level + 1);
}
 
void printSymbolTable() {
    printf("Symbol Table:\n");
    for (int i = 0; i < symbol_count; i++) {
        printf("%s = %d\n", symbol_table[i].name, symbol_table[i].value);
    }
}
 
%}

%locations

%union {
    int intVal;
    char *strVal;
    struct ASTNode *astNode;
}
 
%token <intVal> NUMBER
%token <strVal> IDENTIFIER
%token ADD SUB MUL DIV
%token LPAREN RPAREN EOL ASSIGN
%type <astNode> exp term factor program
 
%right ASSIGN
%left ADD SUB
%left MUL DIV
%nonassoc EOL
 
%%
 
program : exp EOL { 
            $$ = $1; 
            $$->value = evaluateNode($1); 
            printf("Result: %d\n", $$->value); 
            printAST($1, 0); 
        }
        | program exp EOL { 
            $$ = $2; 
            $$->value = evaluateNode($2); 
            printf("Result: %d\n", $$->value); 
            printAST($2, 0); 
        }
        | error { yyerrok; yyclearin;}
        ;
 
exp     : term { $$ = $1; $$->value = evaluateNode($1); }
        | exp ADD term { 
            $$ = createNode(NODE_ADD, 0, NULL, $1, $3); 
            $$->value = evaluateNode($1) + evaluateNode($3); 
        }
        | exp SUB term { 
            $$ = createNode(NODE_SUB, 0, NULL, $1, $3); 
            $$->value = evaluateNode($1) - evaluateNode($3); 
        }
        | IDENTIFIER ASSIGN exp {
            set_variable_value($1, evaluateNode($3)); 
            $$ = createNode(NODE_ASSIGN, 0, strdup($1), NULL, $3);
            $$->value = $3->value;
        }
        ;
 
term    : factor { $$ = $1; $$->value = evaluateNode($1); }
        | term MUL factor { 
            $$ = createNode(NODE_MUL, 0, NULL, $1, $3); 
            $$->value = evaluateNode($1) * evaluateNode($3); 
        }
        | term DIV factor { 
            $$ = createNode(NODE_DIV, 0, NULL, $1, $3); 
            $$->value = evaluateNode($1) / evaluateNode($3); 
        }
        ;
 
factor  : NUMBER { $$ = createNode(NODE_NUMBER, $1, NULL, NULL, NULL); }
        | IDENTIFIER { 
            $$ = createNode(NODE_IDENTIFIER, get_variable_value($1), strdup($1), NULL, NULL); 
        }
        | LPAREN exp RPAREN { 
            $$ = createNode(NODE_PAREN, 0, NULL, $2, NULL); 
            $$->value = evaluateNode($2); 
        }
        ;
 
%%

void yyerror(char *str){
    printf("ERROR: %s\n",str);
}

int main() {
    printf("Enter expression: \n");
    yyparse();
    printSymbolTable();
    return 0;
}