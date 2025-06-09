#pragma once
#include "Container.h"
#include "NameHandle.h"

namespace Thunder
{
    enum class basal_ast_node_type : uint8
    {
        function,
        func_signature,
        param_list,
        param,
        statement_list,
        statement,
        expression,
        type
    };

    enum class statement_type : uint8
    {
        declare,
        assign,
        return_,
        undefined
    };

    enum class expression_type : uint8
    {
        binary_op,
        priority,
        identifier,
        integer,
        undefined
    };

    enum class var_type : uint8
    {
        tp_int,
        tp_float,
        tp_void,
        undefined
    };

    enum class binary_op : uint8
    {
        add,
        sub,
        mul,
        div,
        undefined
    };

    struct parse_node
    {
        int int_val;
        char *str_val;
        class ast_node *token;
    };

    class ast_node
    {
    public:
        ast_node(basal_ast_node_type type) : Type(type) {}
        virtual ~ast_node() = default;

        virtual void GenerateHLSL(String& outResult) {};
        virtual void GenerateDXIL();
        virtual void PrintAST(int indent) {};

    public:
        basal_ast_node_type Type;
    };

    class ast_node_function : public ast_node
    {
    public:
        ast_node_function() : ast_node(basal_ast_node_type::function) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        class ast_node_func_signature* Signature = nullptr;
        class ast_node_statement_list* Body = nullptr;
    };

    class ast_node_func_signature : public ast_node
    {
    public:
        ast_node_func_signature() : ast_node(basal_ast_node_type::func_signature) {}

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
        ast_node_param_list() : ast_node(basal_ast_node_type::param_list) {}

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
        ast_node_param() : ast_node(basal_ast_node_type::param) {}

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
        ast_node_statement_list() : ast_node(basal_ast_node_type::statement_list) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        class ast_node_statement* StatementsHead = nullptr;
        ast_node_statement* StatementsTail = nullptr;
    };

    class ast_node_statement : public ast_node
    {
    public:
        ast_node_statement(statement_type statType)
        : ast_node(basal_ast_node_type::statement), StatNodeType(statType) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        statement_type StatNodeType;
        ast_node_statement* NextStatement = nullptr;
    };

    class ast_node_var_declaration : public ast_node_statement
    {
    public:
        ast_node_var_declaration() : ast_node_statement(statement_type::declare) {}

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
        ast_node_assignment() : ast_node_statement(statement_type::assign) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        NameHandle LhsVar = nullptr;
        ast_node_expression* RhsExpression = nullptr;
    };

    class ast_node_return : public ast_node_statement
    {
    public:
        ast_node_return() : ast_node_statement(statement_type::return_) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        ast_node_expression* RetValue = nullptr;
    };

    class ast_node_expression : public ast_node
    {
    public:
        ast_node_expression(expression_type exprType)
            : ast_node(basal_ast_node_type::expression), ExprType(exprType) {}
    public:
        expression_type ExprType;
    };

    class ast_node_binary_operation : public ast_node_expression
    {
    public:
        ast_node_binary_operation() : ast_node_expression (expression_type::binary_op) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        binary_op op = binary_op::undefined;
        ast_node_expression* left = nullptr;
        ast_node_expression* right = nullptr;
    };

    class ast_node_priority : public ast_node_expression
    {
    public:
        ast_node_priority() : ast_node_expression(expression_type::priority) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        ast_node_expression * Content = nullptr;
    };

    class ast_node_identifier : public ast_node_expression
    {
    public:
        ast_node_identifier() : ast_node_expression(expression_type::identifier) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        NameHandle Identifier = nullptr;
    };

    class ast_node_integer : public ast_node_expression
    {
    public:
        ast_node_integer() : ast_node_expression(expression_type::integer) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        int IntValue = 0;
    };

    class ast_node_type : public ast_node
    {
    public:
        ast_node_type() : ast_node(basal_ast_node_type::type) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        var_type ParamType = var_type::undefined;
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
    ast_node* create_binary_op_node(binary_op op, ast_node* left, ast_node* right);
    ast_node* create_priority_node(ast_node* expr);
    ast_node* create_identifier_node(const char* name);
    ast_node* create_int_literal_node(int value);
    ast_node* create_type_node(var_type type);
    void post_process_ast(ast_node* nodeRoot);

}
