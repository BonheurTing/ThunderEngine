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
        undefined
    };

    enum class enum_expr_type : uint8
    {
        binary_op, // 二元运算
        shuffle, // 向量分量重排
        component, // 成员访问
        reference, // 出现过的
        integer,
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
        String text = "";
        size_t length = 0;
    };

    struct parse_node
    {
        union
        {
            class ast_node* node;
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
        String type_name; // 用于存储类型名称

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
        ast_node_block* body;
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
            : ast_node(enum_ast_node_type::expression), expr_type(exprType) {}
    public:
        enum_expr_type expr_type;
        ast_node* expr_data_type = nullptr; // 用于存储表达式的数据类型
    };

    class binary_op_expression : public ast_node_expression
    {
    public:
        binary_op_expression() : ast_node_expression (enum_expr_type::binary_op) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
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
        reference_expression(String id, ast_node* ref)
            : ast_node_expression(enum_expr_type::reference)
            , identifier(std::move(id)), target(ref) {}
        
        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    private:
        String identifier;
        ast_node* target = nullptr; // 指向符号表中的节点
    };

    ast_node* create_type_node(const token_data& type_info);
    ast_node* create_pass_node(ast_node* struct_node, ast_node* stage_node);

    int tokenize(token_data& t, const parse_location* loc, const char* text, int text_len, int token);
    void post_process_ast(ast_node* nodeRoot);
}
