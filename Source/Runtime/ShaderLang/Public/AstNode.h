#pragma once
#include "Container.h"
#include "NameHandle.h"

namespace Thunder
{
    enum class EBasalNodeType : uint8
    {
        NODE_FUNCTION,
        NODE_FUNC_SIGNATURE,
        NODE_PARAM_LIST,
        NODE_PARAM,
        NODE_STATEMENT_LIST,
        NODE_STATEMENT,
        NODE_EXPRESSION,
        NODE_TYPE
    };

    enum class EStatementType : uint8
    {
        STAT_DECLARE,
        STAT_ASSIGN,
        STAT_RETURN,
        STAT_UNDEFINED
    };

    enum class EExpressionType : uint8
    {
        EXPR_BINARY_OP,
        EXPR_PRIORITY,
        EXPR_IDENTIFIER,
        EXPR_INTEGER,
        EXPR_UNDEFINED
    };

    enum class EVarType : uint8
    {
        TP_INT,
        TP_FLOAT,
        TP_VOID,
        TP_UNDEFINED
    };

    enum class EBinaryOp : uint8
    {
        OP_ADD,
        OP_SUB,
        OP_MUL,
        OP_DIV,
        OP_UNDEFINED
    };

    struct parse_node
    {
        int intVal;
        char *strVal;
        class ast_node *astNode;
    };

    class ast_node
    {
    public:
        ast_node(EBasalNodeType type) : Type(type) {}
        virtual ~ast_node() = default;

        virtual void GenerateHLSL(String& outResult) {};
        virtual void GenerateDXIL();
        virtual void PrintAST(int indent) {};

    public:
        EBasalNodeType Type;
    };

    class ast_node_function : public ast_node
    {
    public:
        ast_node_function() : ast_node(EBasalNodeType::NODE_FUNCTION) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        class ast_node_func_signature* Signature = nullptr;
        class ast_node_statement_list* Body = nullptr;
    };

    class ast_node_func_signature : public ast_node
    {
    public:
        ast_node_func_signature() : ast_node(EBasalNodeType::NODE_FUNC_SIGNATURE) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        class ast_node_type* ReturnType = nullptr;
        NameHandle FuncName = nullptr;
        class ast_node_param_list* Params = nullptr;
    };

    class ast_node_param_list : public ast_node
    {
    public:
        ast_node_param_list() : ast_node(EBasalNodeType::NODE_PARAM_LIST) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        int ParamCount = 0;
        class ast_node_param* ParamsHead = nullptr;
        ast_node_param* ParamsTail = nullptr;
    };

    class ast_node_param : public ast_node
    {
    public:
        ast_node_param() : ast_node(EBasalNodeType::NODE_PARAM) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        ast_node_type* ParamType = nullptr;
        NameHandle ParamName = nullptr;
        ast_node_param*  NextParam = nullptr;
    };

    class ast_node_statement_list : public ast_node
    {
    public:
        ast_node_statement_list() : ast_node(EBasalNodeType::NODE_STATEMENT_LIST) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        class ast_node_statement* StatementsHead = nullptr;
        ast_node_statement* StatementsTail = nullptr;
    };

    class ast_node_statement : public ast_node
    {
    public:
        ast_node_statement(EStatementType statType)
        : ast_node(EBasalNodeType::NODE_STATEMENT), StatNodeType(statType) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        EStatementType StatNodeType;
        ast_node_statement* NextStatement = nullptr;
    };

    class ast_node_var_declaration : public ast_node_statement
    {
    public:
        ast_node_var_declaration() : ast_node_statement(EStatementType::STAT_DECLARE) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        ast_node_type* VarDelType = nullptr;
        NameHandle VarName = nullptr;
        class ast_node_expression* DelExpression = nullptr;
    };

    class ast_node_assignment : public ast_node_statement
    {
    public:
        ast_node_assignment() : ast_node_statement(EStatementType::STAT_ASSIGN) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        NameHandle LhsVar = nullptr;
        ast_node_expression* RhsExpression = nullptr;
    };

    class ast_node_return : public ast_node_statement
    {
    public:
        ast_node_return() : ast_node_statement(EStatementType::STAT_RETURN) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        ast_node_expression* RetValue = nullptr;
    };

    class ast_node_expression : public ast_node
    {
    public:
        ast_node_expression(EExpressionType exprType)
            : ast_node(EBasalNodeType::NODE_EXPRESSION), ExprType(exprType) {}
    public:
        EExpressionType ExprType;
    };

    class ast_node_binary_operation : public ast_node_expression
    {
    public:
        ast_node_binary_operation() : ast_node_expression (EExpressionType::EXPR_BINARY_OP) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        EBinaryOp op = EBinaryOp::OP_UNDEFINED;
        ast_node_expression* left = nullptr;
        ast_node_expression* right = nullptr;
    };

    class ast_node_priority : public ast_node_expression
    {
    public:
        ast_node_priority() : ast_node_expression(EExpressionType::EXPR_PRIORITY) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        ast_node_expression * Content = nullptr;
    };

    class ast_node_identifier : public ast_node_expression
    {
    public:
        ast_node_identifier() : ast_node_expression(EExpressionType::EXPR_IDENTIFIER) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        NameHandle Identifier = nullptr;
    };

    class ast_node_integer : public ast_node_expression
    {
    public:
        ast_node_integer() : ast_node_expression(EExpressionType::EXPR_INTEGER) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        int IntValue = 0;
    };

    class ast_node_type : public ast_node
    {
    public:
        ast_node_type() : ast_node(EBasalNodeType::NODE_TYPE) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        EVarType ParamType = EVarType::TP_UNDEFINED;
    };

    ast_node* create_function_node(ast_node* signature, ast_node* body);
    ast_node* create_func_signature_node(ast_node* returnTypeNode, const char* name, ast_node* params);
    ast_node* create_param_list_node();
    void add_param_to_list(ast_node* list, ast_node* param);
    ast_node* create_param_node(ast_node* typeNode, const char* name);
    ast_node* create_statement_list_node();
    void add_statement_to_list(ast_node* list, ast_node* stmt);
    ast_node* create_var_decl_node(ast_node* typeNode, const char* name, ast_node* init_expr);
    ast_node* create_assignment_node(const char* lhs, ast_node* rhs);
    ast_node* create_return_node(ast_node* expr);
    ast_node* create_binary_op_node(EBinaryOp op, ast_node* left, ast_node* right);
    ast_node* create_priority_node(ast_node* expr);
    ast_node* create_identifier_node(const char* name);
    ast_node* create_int_literal_node(int value);
    ast_node* create_type_node(EVarType type);
    void post_process_ast(ast_node* nodeRoot);

}
