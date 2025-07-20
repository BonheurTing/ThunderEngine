#pragma once
#include "Assertion.h"
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
        block,
        statement,
        expression
    };

    enum class enum_statement_type : uint8
    {
        declare,
        assign,
        return_,
        condition,
        undefined
    };

    enum class enum_expr_type : uint8
    {
        binary_op, // 二元运算
        shuffle, // 向量分量重排
        component, // 成员访问
        reference, // 出现过的
        integer,
        constant_float,
        constant_bool,
        undefined
    };

    enum class enum_basic_type : uint8
    {
        tp_int,
        tp_float,
        tp_void,
        tp_bool,
        tp_vector,
        tp_matrix,
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
        // 逻辑运算
        logical_and,
        logical_or,
        // 比较运算
        equal,
        not_equal,
        less,
        less_equal,
        greater,
        greater_equal,
        undefined
    };

    struct parse_location
    {
        int first_line;
        int first_column;
        int first_source;
        int last_line;
        int last_column;
        int last_source;

        // is null.
        FORCEINLINE bool is_null() const { return first_line == 0; }
        // set null.
        FORCEINLINE void set_null() { first_line = 0; first_column = 0; first_source = 0; last_line = 0; last_column = 0; last_source = 0; }
    };

    struct token_data : public parse_location
    {
        int token_id = 0;
        String text;
        size_t length = 0;
    };

    struct parse_node
    {
        union
        {
            class ast_node* node;
            class ast_node_type* type;
            class ast_node_block* block;
            class ast_node_statement* statement;
            class ast_node_expression* expression;
        };
        token_data token;
        
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

    // 类型基类，类型相关的用法都在这里 = type_spec
    class ast_node_type : public ast_node
    {
    public:
        ast_node_type() : ast_node(enum_ast_node_type::type) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    public:
        String type_name; // 用于存储类型名称
        // format
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
        NameHandle name = nullptr; // 用于存储变量名称
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
        ast_node_struct(ast_node* token)
        : ast_node(enum_ast_node_type::structure)
        {
            TAssert(token->Type == enum_ast_node_type::type);
            type = static_cast<ast_node_type*>(token);
        }

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
        void add_member(ast_node_variable* mem)
        {
            members.push_back(mem);
        }
    private:
        ast_node_type* type = nullptr;
        TArray<ast_node_variable*> members;
    };

    class ast_node_function : public ast_node
    {
    public:
        ast_node_function(const String& name)
        : ast_node(enum_ast_node_type::function), func_name(name) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
        void set_return_type(ast_node_type* ret_type)
        {
            return_type = ret_type;
        }
        void add_parameter(ast_node_variable* param)
        {
            params.push_back(param);
        }
        void set_body(class ast_node_block* body_statements)
        {
            body = body_statements;
        }
    private:
        // func_signature
        ast_node_type* return_type = nullptr;
        NameHandle func_name = nullptr;
        TArray<ast_node_variable*> params;
        // body
        ast_node_block* body = nullptr;
    };

    class ast_node_block : public ast_node
    {
    public:
        ast_node_block() : ast_node(enum_ast_node_type::block) {}
        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    
        void add_statement(ast_node_statement* statement)
        {
            statements.push_back(statement);
        }
    private:
        TArray<ast_node_statement*> statements; // 存储语句列表
    };

    class ast_node_statement : public ast_node
    {
    public:
        ast_node_statement(enum_statement_type type)
        : ast_node(enum_ast_node_type::statement), stat_type(type) {}
    public:
        enum_statement_type stat_type;
    };

    class variable_declaration_statement : public ast_node_statement
    {
    public:
        variable_declaration_statement(ast_node_type* type, String text, ast_node_expression* expr) noexcept
        : ast_node_statement(enum_statement_type::declare), var_type(type), var_name(std::move(text)), decl_expr(expr) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    private:
        ast_node_type* var_type = nullptr;
        String var_name;
        ast_node_expression* decl_expr = nullptr;
    };

    class assignment_statement : public ast_node_statement
    {
    public:
        assignment_statement(String text, ast_node_expression* expr) noexcept
        : ast_node_statement(enum_statement_type::assign), lhs_var(std::move(text)), rhs_expr(expr) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    private:
        String lhs_var;
        ast_node_expression* rhs_expr = nullptr;
    };

    class return_statement : public ast_node_statement
    {
    public:
        return_statement(ast_node_expression* expr) noexcept
        : ast_node_statement(enum_statement_type::return_), ret_value(expr) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    private:
        ast_node_expression* ret_value = nullptr;
    };

    class condition_statement : public ast_node_statement
    {
    public:
        condition_statement(ast_node_expression* expr, ast_node_statement* true_stmt, ast_node_statement* false_stmt) noexcept
        : ast_node_statement(enum_statement_type::condition), condition(expr), true_branch(true_stmt), false_branch(false_stmt) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    public:
        ast_node_expression* condition = nullptr;
        ast_node_statement* true_branch = nullptr;
        ast_node_statement* false_branch = nullptr;
    };

    // 表达式求值结果类型
    enum class enum_eval_result_type : uint8
    {
        undetermined,     // 未确定
        constant_int,     // 常量整数
        constant_float,   // 常量浮点数
        constant_bool,    // 常量布尔值
        variable,         // 变量
        temp_var,         // 临时变量
        object            // 对象
    };

    // 表达式求值结果
    struct evaluate_expr_result  // NOLINT(cppcoreguidelines-pro-type-member-init)
    {
        ast_node_type* type = nullptr;           // 表达式类型
        enum_eval_result_type result_type = enum_eval_result_type::undetermined;
        union
        {
            ast_node* result_node;     // 结果节点
            int int_value;             // 整数值
            float float_value;         // 浮点值
            bool bool_value;           // 布尔值
        };

        evaluate_expr_result() noexcept = default;  // NOLINT(cppcoreguidelines-pro-type-member-init)

        evaluate_expr_result(ast_node* node) noexcept  // NOLINT(cppcoreguidelines-pro-type-member-init)
            : result_type(enum_eval_result_type::variable), result_node(node) {}

        evaluate_expr_result(int value) noexcept   // NOLINT(cppcoreguidelines-pro-type-member-init)
            : result_type(enum_eval_result_type::constant_int), int_value(value) {}
        
        evaluate_expr_result(float value) noexcept  // NOLINT(cppcoreguidelines-pro-type-member-init)
            : result_type(enum_eval_result_type::constant_float), float_value(value) {}
        
        evaluate_expr_result(bool value) noexcept  // NOLINT(cppcoreguidelines-pro-type-member-init)
            : result_type(enum_eval_result_type::constant_bool), bool_value(value) {}

        
    };

    class ast_node_expression : public ast_node
    {
    public:
        ast_node_expression(enum_expr_type exprType)
            : ast_node(enum_ast_node_type::expression), expr_type(exprType) {}
        
        // 表达式评估函数
        virtual evaluate_expr_result evaluate();
        
    public:
        enum_expr_type expr_type;
        ast_node* expr_data_type = nullptr; // 用于存储表达式的数据类型
    };

    class binary_op_expression : public ast_node_expression
    {
    public:
        binary_op_expression() noexcept
        : ast_node_expression (enum_expr_type::binary_op) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
        evaluate_expr_result evaluate() override;
    public:
        enum_binary_op op = enum_binary_op::undefined;
        ast_node_expression* left = nullptr;
        ast_node_expression* right = nullptr;
    };

    class shuffle_expression : public ast_node_expression
    {
    public:
        shuffle_expression(ast_node_expression* expr, const char in_order[4]) noexcept
        : ast_node_expression(enum_expr_type::shuffle), prefix(expr)
        {
            order[0] = in_order[0];
            order[1] = in_order[1];
            order[2] = in_order[2];
            order[3] = in_order[3];
        }

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    public:
        ast_node_expression* prefix = nullptr;
        char order[4]{ 0, 0, 0, 0 };
    };

    class component_expression : public ast_node_expression
    {
    public:
        component_expression(ast_node_expression* expr, ast_node* identifier, const String& text) noexcept
            : ast_node_expression(enum_expr_type::component),
            component_name(expr), component_member(identifier), member_text(text) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    private:
        ast_node_expression* component_name = nullptr;
        ast_node* component_member = nullptr;
        String member_text;
    };

    /*
     * 出现过的id
     */
    class reference_expression : public ast_node_expression
    {
    public:
        reference_expression(String id, ast_node* ref) noexcept
            : ast_node_expression(enum_expr_type::reference)
            , identifier(std::move(id)), target(ref) {}
        
        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
        evaluate_expr_result evaluate() override;
    private:
        String identifier;
        ast_node* target = nullptr; // 指向符号表中的节点
    };

    // 整数常量表达式
    class constant_int_expression : public ast_node_expression
    {
    public:
        constant_int_expression(int value) noexcept
            : ast_node_expression(enum_expr_type::integer), value(value) {}
        
        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
        evaluate_expr_result evaluate() override;
    private:
        int value;
    };

    // 浮点数常量表达式
    class constant_float_expression : public ast_node_expression
    {
    public:
        constant_float_expression(float value) noexcept
            : ast_node_expression(enum_expr_type::constant_float), value(value) {}
        
        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
        evaluate_expr_result evaluate() override;
    private:
        float value;
    };

    // 布尔常量表达式
    class constant_bool_expression : public ast_node_expression
    {
    public:
        constant_bool_expression(bool value) noexcept
            : ast_node_expression(enum_expr_type::constant_bool), value(value) {}
        
        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
        evaluate_expr_result evaluate() override;
    private:
        bool value;
    };

    int tokenize(token_data& t, const parse_location* loc, const char* text, int text_len, int token);
}
