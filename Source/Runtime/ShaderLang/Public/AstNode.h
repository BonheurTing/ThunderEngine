#pragma once
#include "Container.h"
#include "NameHandle.h"

namespace Thunder
{
    enum class enum_ast_node_type : uint8
    {
        type,
        variable,
        pass,
        structure,
        function,
        func_signature,
        param_list,
        param,
        statement_list,
        statement,
        expression
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

    enum class enum_basic_type : uint8
    {
        tp_int,
        tp_float,
        tp_void,
        tp_bool,
        tp_struct,
        tp_buffer,
        tp_texture,
        tp_sampler,
        undefined
    };
    static_assert(static_cast<size_t>(enum_basic_type::undefined) <= 16, "enum basic type check.");

    enum class enum_texture_type : uint8
    {
        texture1d,
        texture2d,
        texture3d,
        texture_cube,
        texture1d_array,
        texture2d_array,
        texture2d_ms,
        texture2d_ms_array,
        texture_cube_array,
        
        texture_sampler,
        texture_unknown
    };
    static_assert(static_cast<size_t>(enum_texture_type::texture_unknown) <= 16, "enum texture type check.");

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

    // 类型基类，类型相关的用法都在这里
    class ast_node_type : public ast_node
    {
    public:
        ast_node_type() : ast_node(enum_ast_node_type::type) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    public:
        enum_basic_type param_type : 4 = enum_basic_type::undefined;
        enum_texture_type texture_type : 4 = enum_texture_type::texture_unknown;

        bool is_semantic : 1 = false; // 是否是shader语义
    };

    // 变量基类，包含参数变量，局部变量，全局变量，shader semantic等
    class ast_node_variable : public ast_node
    {
    public:
        ast_node_variable(const String& name)
            : ast_node(enum_ast_node_type::variable), name(name) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    public:
        ast_node_type* type = nullptr;
        NameHandle name = nullptr;
        NameHandle semantic = nullptr; // 用于存储shader语义
        String value; // 用于存储常量值或默认值
    };

    class ast_node_pass : public ast_node
    {
    public:
        ast_node_pass() : ast_node(enum_ast_node_type::pass) {}
        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    public:
        class ast_node_struct* structure = nullptr;
        class ast_node_function* stage = nullptr;
    };

    class ast_node_struct : public ast_node
    {
    public:
        ast_node_struct(const String& name)
        : ast_node(enum_ast_node_type::structure), name(name) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
        void add_member(ast_node_variable* name)
        {
            members.push_back(name);
        }
    public:
        NameHandle name = nullptr;
        TArray<ast_node_variable*> members;
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

    //todo: 终结符，待删除
    class ast_node_identifier : public ast_node_expression
    {
    public:
        ast_node_identifier() : ast_node_expression(enum_expr_type::identifier) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    public:
        NameHandle Identifier = nullptr;
    };

    //todo: 终结符，待删除
    class ast_node_integer : public ast_node_expression
    {
    public:
        ast_node_integer() : ast_node_expression(enum_expr_type::integer) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    public:
        int IntValue = 0;
    };

    ast_node* create_type_node(enum_basic_type type);
    ast_node* create_pass_node(ast_node* struct_node, ast_node* stage_node);
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
    void post_process_ast(ast_node* nodeRoot);
}
