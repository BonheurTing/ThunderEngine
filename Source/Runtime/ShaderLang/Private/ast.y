%{
#include "../ast.h"

extern FILE *yyin;
 
int yylex(void);
void yyerror(char *);
 
 
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

int main(int argc, char *argv[]) {
    if (argc == 1)
    {
        yyparse();
    }
    else if (argc == 3 && strcmp(argv[1], "-f") == 0)
    {
        yyin = fopen(argv[2], "r");
        if (!yyin)
        {
            yyerror("open failed");
            return 0;
        }
        yyparse();
        fclose(yyin);
    }
    else
    {
        yyerror("undefined parameter");
        return 0;
    }
    printSymbolTable();
    return 0;
}