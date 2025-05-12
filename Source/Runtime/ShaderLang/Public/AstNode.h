
#ifndef ASTNODE_H
#define ASTNODE_H

typedef enum {
    NODE_FUNCTION,
    NODE_FUNC_SIGNATURE,
    NODE_PARAM_LIST,
    NODE_PARAM,
    NODE_STATEMENT_LIST,
    NODE_STATEMENT,
    NODE_VAR_DECL,
    NODE_ASSIGNMENT,
    NODE_RETURN,
    NODE_EXPRESSION,
    NODE_TYPE,
    NODE_IDENTIFIER,
    NODE_INT_LITERAL,
    NODE_BINARY_OP
} NodeType;

typedef enum
{
    Stat_DECLARE,
    Stat_ASSIGN,
    Stat_RETURN
} StatType;

typedef enum {
    TP_INT,
    TP_FLOAT,
    TP_VOID
} VarType;

typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV
} BinaryOp;

typedef struct ASTNode {
    NodeType type;
    union {
        // For function
        struct {
            ASTNode* Signature;
            ASTNode* Body;
        };
        
        // For function signature
        struct {
            VarType ReturnType;
            char* FuncName;
            ASTNode* Params;
        };

        // For parameter list
        struct {
            int ParamCount;
            ASTNode* ParamsHead;
            ASTNode* ParamsTail;
        };
        
        // For parameter
        struct {
            VarType ParamType;
            char* ParamName;
            ASTNode*  NextParam;
        };

        // For statement list
        struct {
            ASTNode* StatementsHead;
            ASTNode* StatementsTail;
        };

        // For statement
        struct {
            StatType StatNodeType;
            ASTNode* StatContent;
            ASTNode* NextStatement;
        };
        
        // For variable declaration
        struct {
            VarType VarDelType;
            char* VarName;
            ASTNode* DelExpression;
        };
        
        // For assignment
        struct {
            char* lhs;
            ASTNode* rhs;
        };

        // For return statement
        struct {
            ASTNode* RetValue;
        };
        
        // For binary operation
        struct {
            BinaryOp op;
            ASTNode* left;
            ASTNode* right;
        };
        
        // For literals and identifiers
        int int_value;
        char* string_value;
    };
} ASTNode;

ASTNode* create_function_node(ASTNode* signature, ASTNode* body);
ASTNode* create_func_signature_node(const ASTNode* returnTypeNode, const char* name, ASTNode* params);
ASTNode* create_param_list_node();
void add_param_to_list(ASTNode* list, ASTNode* param);
ASTNode* create_param_node(const ASTNode* typeNode, const char* name);
ASTNode* create_statement_list_node();
void add_statement_to_list(ASTNode* list, ASTNode* stmt);
ASTNode* create_statement_node(ASTNode* statContent, StatType type);
ASTNode* create_var_decl_node(const ASTNode* typeNode, char* name, ASTNode* init_expr);
ASTNode* create_assignment_node(char* lhs, ASTNode* rhs);
ASTNode* create_return_node(ASTNode* expr);
ASTNode* create_expression_node();
ASTNode* create_type_node(VarType type);
ASTNode* create_identifier_node(char* name);
ASTNode* create_int_literal_node(int value);
ASTNode* create_binary_op_node(BinaryOp op, ASTNode* left, ASTNode* right);
void print_ast(ASTNode* node, int indent);

#endif