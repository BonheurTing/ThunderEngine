#pragma optimize("", off)
#include "AstNode.h"

#include <cstdio>
#include <cstdlib>
#include <string>

static void print_blank(int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
}

static const char* get_type_name(VarType type) {
    switch (type) {
        case TP_INT: return "int";
        case TP_FLOAT: return "float";
        case TP_VOID: return "void";
        default: return "unknown";
    }
}

ASTNode* create_function_node(ASTNode* signature, ASTNode* body) {
    const auto node = static_cast<ASTNode*>(malloc(sizeof(ASTNode)));
    node->type = NODE_FUNCTION;
    node->Signature = signature;
    node->Body = body;
    return node;
}

ASTNode* create_func_signature_node(const ASTNode* returnTypeNode, const char *name, ASTNode* params) {
    if (returnTypeNode == nullptr || returnTypeNode->type != NODE_TYPE) {
        fprintf(stderr, "Error: Invalid return type node for function signature.\n");
        return nullptr;
    }
    const auto node = static_cast<ASTNode*>(malloc(sizeof(ASTNode)));
    node->type = NODE_FUNC_SIGNATURE;
    node->ReturnType = returnTypeNode->ParamType;
    node->FuncName = strdup(name);
    node->Params = params;
    return node;
}

ASTNode* create_param_list_node() {
    const auto node = static_cast<ASTNode*>(malloc(sizeof(ASTNode)));
    node->type = NODE_PARAM_LIST;
    node->ParamCount = 0;
    node->ParamsHead = nullptr;
    node->ParamsTail = nullptr;
    return node;
}

void add_param_to_list(ASTNode* list, ASTNode* param) {
    // check if the list and param are valid
    const bool isValid = list != nullptr && param != nullptr && 
                  (list->type == NODE_PARAM_LIST) && (param->type == NODE_PARAM);
    if (!isValid) {
        fprintf(stderr, "Error: Attempting to add non-parameter or to non-parameter list node.\n");
        return;
    }
    // add param to the list
    if (list->ParamsHead == nullptr && list->ParamsTail == nullptr) {
        list->ParamsHead = param;
        list->ParamsTail = param;
    } else {
        list->ParamsTail->NextParam = param;
        list->ParamsTail = param;
    }
    list->ParamCount++;
}

ASTNode* create_param_node(const ASTNode* typeNode, const char *name) {
    if(typeNode == nullptr || typeNode->type != NODE_TYPE) {
        fprintf(stderr, "Error: Invalid type node for parameter.\n");
        return nullptr;
    }
    const auto node = static_cast<ASTNode*>(malloc(sizeof(ASTNode)));
    node->type = NODE_PARAM;
    node->ParamType = typeNode->ParamType;
    node->ParamName = strdup(name);
    node->NextParam = nullptr;
    return node;
}

ASTNode* create_statement_list_node() {
    const auto node = static_cast<ASTNode*>(malloc(sizeof(ASTNode)));
    node->type = NODE_STATEMENT_LIST;
    node->StatementsHead = nullptr;
    node->StatementsTail = nullptr;
    return node;
}

void add_statement_to_list(ASTNode* list, ASTNode* stmt)
{
    if (list == nullptr || stmt == nullptr || list->type != NODE_STATEMENT_LIST) {
        fprintf(stderr, "Error: Attempting to add statement or to non-statement list node.\n");
        return;
    }
    if (list->StatementsHead == nullptr && list->StatementsTail == nullptr) {
        list->StatementsHead = stmt;
        list->StatementsTail = stmt;
    } else {
        list->StatementsTail->NextStatement = stmt;
        list->StatementsTail = stmt;
    }
}

ASTNode* create_statement_node(ASTNode* statContent, StatType type)
{
    const auto node = static_cast<ASTNode*>(malloc(sizeof(ASTNode)));
    node->type = NODE_STATEMENT;
    node->StatNodeType = type;
    node->StatContent = statContent;
    node->NextParam = nullptr;
    return node;
}

ASTNode* create_var_decl_node(const ASTNode* typeNode, char *name, ASTNode* init_expr) {
    const auto node = static_cast<ASTNode*>(malloc(sizeof(ASTNode)));
    node->type = NODE_VAR_DECL;
    node->VarDelType = typeNode->ParamType;
    node->VarName = strdup(name);
    node->DelExpression = init_expr;
    return node;
}

ASTNode* create_assignment_node(char *lhs, ASTNode* rhs) {
    const auto node = static_cast<ASTNode*>(malloc(sizeof(ASTNode)));
    node->type = NODE_ASSIGNMENT;
    node->lhs = strdup(lhs);
    node->rhs = rhs;
    return node;
}

ASTNode* create_return_node(ASTNode* expr)
{
    const auto node = static_cast<ASTNode*>(malloc(sizeof(ASTNode)));
    node->type = NODE_RETURN;
    node->RetValue = expr;
    return node;
}

ASTNode* create_expression_node() {
    const auto node = static_cast<ASTNode*>(malloc(sizeof(ASTNode)));
    node->type = NODE_EXPRESSION;
    return node;
}

ASTNode* create_type_node(VarType type) {
    const auto node = static_cast<ASTNode*>(malloc(sizeof(ASTNode)));
    node->type = NODE_TYPE;
    node->ParamType = type; // 复用param_type字段
    return node;
}

ASTNode* create_identifier_node(char *name) {
    const auto node = static_cast<ASTNode*>(malloc(sizeof(ASTNode)));
    node->type = NODE_IDENTIFIER;
    node->string_value = strdup(name);
    return node;
}

ASTNode* create_int_literal_node(int value) {
    const auto node = static_cast<ASTNode*>(malloc(sizeof(ASTNode)));
    node->type = NODE_INT_LITERAL;
    node->int_value = value;
    return node;
}

ASTNode* create_binary_op_node(BinaryOp op, ASTNode* left, ASTNode* right) {
    const auto node = static_cast<ASTNode*>(malloc(sizeof(ASTNode)));
    node->type = NODE_BINARY_OP;
    node->op = op;
    node->left = left;
    node->right = right;
    return node;
}

void print_ast(ASTNode* node, int indent) {
    if (!node) return;
    print_blank(indent);

    switch (node->type)
    {
    case NODE_FUNCTION:
    {
        printf("Function:\n");
        print_ast(node->Signature, indent + 1);
        print_ast(node->Body, indent + 1);
        break;
    }
            
    case NODE_FUNC_SIGNATURE:
    {
        printf("Signature:\n");
        print_blank(indent + 1);
        printf("FuncName: %s\n", node->FuncName);
        print_blank(indent + 1);
        printf("ReturnType: %s\n", get_type_name(node->ReturnType));
        print_ast(node->Params, indent + 1);
        break;
    }

    case NODE_PARAM_LIST:
    {
        printf("ParamList(Count %d):\n", node->ParamCount);
        print_ast(node->ParamsHead, indent + 1);
        break;
    }

    case NODE_PARAM:
    {
        printf("Param { Type : %s; Name : %s }\n", get_type_name(node->ParamType), node->ParamName);
        print_ast(node->NextParam, indent);
        break;
    }

    case NODE_STATEMENT_LIST:
    {
        printf("StatementList:\n");
        print_ast(node->StatementsHead, indent + 1);
        break;
    }

    case NODE_STATEMENT:
    {
        print_ast(node->StatContent, indent);
        print_ast(node->NextStatement, indent);
        break;
    }

    case NODE_VAR_DECL:
    {
        printf("VarDecl: {\n");
        print_blank(indent * 2 + 1);
        printf("Type : %s; Name : %s\n", get_type_name(node->VarDelType), node->VarName);
        print_ast(node->DelExpression, indent * 2 + 1);
        print_blank(indent * 2);
        printf("}\n");
        break;
    }
            
    case NODE_ASSIGNMENT:
    {
        printf("Assignment: {\n");
        print_blank(indent * 2 + 1);
        printf("%s = \n", node->lhs);
        print_ast(node->rhs, indent * 2 + 1);
        print_blank(indent * 2);
        printf("}\n");
        break;
    }

    case NODE_EXPRESSION:
    {
        printf("Expression:\n");
        break;
    }
            
    case NODE_BINARY_OP:
    {
        printf("BinaryOp: ");
        switch (node->op) {
        case OP_ADD: printf("+\n"); break;
        case OP_SUB: printf("-\n"); break;
        case OP_MUL: printf("*\n"); break;
        case OP_DIV: printf("/\n"); break;
        }
        print_ast(node->left, indent + 1);
        print_ast(node->right, indent + 1);
        break;
    }
            
    case NODE_INT_LITERAL:
    {
        printf("IntLiteral: %d\n", node->int_value);
        break;
    }
            
    case NODE_IDENTIFIER:
    {
        printf("Identifier: %s\n", node->string_value);
        break;
    }
            
    default:
        printf("Unknown node type\n");
    }
}
#pragma optimize("", on)