#pragma once
#include "Container.h"
#include "NameHandle.h"

namespace Thunder
{
    enum class enum_ast_node_type : uint8
    {
        structure,
        function,
        func_signature,
        param_list,
        param,
        statement_list,
        statement,
        expression,
        type
    };

    enum class enum_struct_type : uint8
    {
        variable,   // 变量
        function,   // 函数
        type,       // 类型
        constant,   // 常量
        undefined   // 未定义符号
    };

    enum class enum_statement_type : uint8
    {
        declare,
        assign,
        return_,
        undefined
    };

    enum class enum_expr_type : uint8
    {
        binary_op,
        priority,
        identifier,
        integer,
        undefined
    };

    enum class enum_var_type : uint8
    {
        tp_int,
        tp_float,
        tp_void,
        undefined
    };

    enum class enum_binary_op : uint8
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
        ast_node(enum_ast_node_type type) : Type(type) {}
        virtual ~ast_node() = default;

        virtual void generate_hlsl(String& outResult) {}
        virtual void generate_dxil();
        virtual void print_ast(int indent) {}

    public:
        enum_ast_node_type Type;
    };

    class ast_node_struct : public ast_node
    {
    public:
        ast_node_struct(const String& name)
        : ast_node(enum_ast_node_type::structure), struct_name(name) {}

        void generate_hlsl(String& outResult) override {};
        void print_ast(int indent) override {};
        void add_member(const String& name)
        {
            member_names.push_back(name);
        }
    public:
        NameHandle struct_name = nullptr;
        TArray<String> member_names;
    };

    class ast_node_function : public ast_node
    {
    public:
        ast_node_function() : ast_node(enum_ast_node_type::function) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    public:
        class ast_node_func_signature* signature = nullptr;
        class ast_node_statement_list* body = nullptr;
    };

    class ast_node_func_signature : public ast_node
    {
    public:
        ast_node_func_signature() : ast_node(enum_ast_node_type::func_signature) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    public:
        class ast_node_type* return_type = nullptr;
        NameHandle func_name = nullptr;
        class ast_node_param_list* params = nullptr;
    };

    class ast_node_param_list : public ast_node
    {
    public:
        ast_node_param_list() : ast_node(enum_ast_node_type::param_list) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    public:
        int param_count = 0;
        class ast_node_param* params_head = nullptr;
        ast_node_param* params_tail = nullptr;
    };

    class ast_node_param : public ast_node
    {
    public:
        ast_node_param() : ast_node(enum_ast_node_type::param) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    public:
        ast_node_type* param_type = nullptr;
        NameHandle param_name = nullptr;
        ast_node_param*  next_param = nullptr;
    };

    class ast_node_statement_list : public ast_node
    {
    public:
        ast_node_statement_list() : ast_node(enum_ast_node_type::statement_list) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    public:
        class ast_node_statement* statements_head = nullptr;
        ast_node_statement* StatementsTail = nullptr;
    };

    class ast_node_statement : public ast_node
    {
    public:
        ast_node_statement(enum_statement_type statType)
        : ast_node(enum_ast_node_type::statement), StatNodeType(statType) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    public:
        enum_statement_type StatNodeType;
        ast_node_statement* NextStatement = nullptr;
    };

    class ast_node_var_declaration : public ast_node_statement
    {
    public:
        ast_node_var_declaration() : ast_node_statement(enum_statement_type::declare) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    public:
        ast_node_type* VarDelType = nullptr;
        NameHandle VarName = nullptr;
        class ast_node_expression* DelExpression = nullptr;
    };

    class ast_node_assignment : public ast_node_statement
    {
    public:
        ast_node_assignment() : ast_node_statement(enum_statement_type::assign) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    public:
        NameHandle LhsVar = nullptr;
        ast_node_expression* RhsExpression = nullptr;
    };

    class ast_node_return : public ast_node_statement
    {
    public:
        ast_node_return() : ast_node_statement(enum_statement_type::return_) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    public:
        ast_node_expression* RetValue = nullptr;
    };

    class ast_node_expression : public ast_node
    {
    public:
        ast_node_expression(enum_expr_type exprType)
            : ast_node(enum_ast_node_type::expression), ExprType(exprType) {}
    public:
        enum_expr_type ExprType;
    };

    class ast_node_binary_operation : public ast_node_expression
    {
    public:
        ast_node_binary_operation() : ast_node_expression (enum_expr_type::binary_op) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    public:
        enum_binary_op op = enum_binary_op::undefined;
        ast_node_expression* left = nullptr;
        ast_node_expression* right = nullptr;
    };

    class ast_node_priority : public ast_node_expression
    {
    public:
        ast_node_priority() : ast_node_expression(enum_expr_type::priority) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    public:
        ast_node_expression * Content = nullptr;
    };

    class ast_node_identifier : public ast_node_expression
    {
    public:
        ast_node_identifier() : ast_node_expression(enum_expr_type::identifier) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    public:
        NameHandle Identifier = nullptr;
    };

    class ast_node_integer : public ast_node_expression
    {
    public:
        ast_node_integer() : ast_node_expression(enum_expr_type::integer) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    public:
        int IntValue = 0;
    };

    class ast_node_type : public ast_node
    {
    public:
        ast_node_type() : ast_node(enum_ast_node_type::type) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    public:
        enum_var_type ParamType = enum_var_type::undefined;
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
    ast_node* create_binary_op_node(enum_binary_op op, ast_node* left, ast_node* right);
    ast_node* create_priority_node(ast_node* expr);
    ast_node* create_identifier_node(const char* name);
    ast_node* create_int_literal_node(int value);
    ast_node* create_type_node(enum_var_type type);
    void post_process_ast(ast_node* nodeRoot);

}
