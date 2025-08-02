#pragma once
#include "Assertion.h"
#include "Container.h"
#include "NameHandle.h"
#include "Templates/RefCounting.h"

namespace Thunder
{
    enum class enum_ast_node_type : uint8
    {
        type,
        variable,
        archive,
        pass,
        structure,
        function,
        statement,
        expression
    };

    enum class enum_statement_type : uint8
    {
        declare,
        assign,
        return_,
        break_,
        continue_,
        discard_,
        expression,
        condition,
        block,
        undefined
    };

    enum class enum_expr_type : uint8
    {
        binary_op, // 二元运算
        unary_op,  // 一元运算
        shuffle, // 向量分量重排
        component, // 成员访问
        index, // 数组索引
        reference, // 出现过的
        integer,
        constant_float,
        constant_bool,
        assignment, // 赋值表达式
        conditional, // 条件表达式
        function_call, // 函数调用
        constructor_call, // 构造函数调用
        compound_assignment, // 复合赋值表达式
        chain, // 链式表达式（逗号表达式）
        cast, // 类型转换表达式
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
        mod,
        // 位运算
        bit_and,
        bit_or,
        bit_xor,
        left_shift,
        right_shift,
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

    enum class enum_unary_op : uint8
    {
        positive,   // +
        negative,   // -
        logical_not, // !
        bit_not,    // ~
        pre_inc,    // ++expr
        pre_dec,    // --expr
        post_inc,   // expr++
        post_dec,   // expr--
        undefined
    };

    enum class enum_type_qualifier : uint8
    {
        none = 0,
        // Input/Output qualifiers
        input = 1 << 0,      // in
        output = 1 << 1,     // out  
        inout = 1 << 2,      // inout
        // Storage class qualifiers
        extern_ = 1 << 3,    // extern
        static_ = 1 << 4,    // static
        uniform = 1 << 5,    // uniform
        // Type modifiers
        const_ = 1 << 6,     // const
        volatile_ = 1 << 7   // volatile (using only 8 bits for now)
    };

    // Type qualifier flags can be combined
    using type_qualifier_flags = uint8;

    enum class enum_assignment_op : uint8
    {
        assign,        // =
        add_assign,    // +=
        sub_assign,    // -=
        mul_assign,    // *=
        div_assign,    // /=
        mod_assign,    // %=
        lshift_assign, // <<=
        rshift_assign, // >>=
        and_assign,    // &=
        or_assign,     // |=
        xor_assign,    // ^=
        undefined
    };

    enum class enum_symbol_type : uint8
    {
        variable,   // 变量 variable_declaration_statement
        type,      // 类型 ast_node_type
        structure,  // 结构体
        function,   // 函数 ast_node_function
        //constant,   // 常量
        undefined  // 未定义符号
    };

    struct shader_lang_symbol
    {
        String name;
        enum_symbol_type type = enum_symbol_type::undefined;
        class ast_node* owner = nullptr; //symbol所在的语法树节点
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

    struct dimensions
    {
        uint32 dimension = 0;
        uint32 array_count[3] = {0};

        _NODISCARD_ bool is_array() const noexcept
        {
            return dimension > 0;
        }
        _NODISCARD_ void add_dimension(uint32 dim) noexcept
        {
            TAssert(dimension < 3);
            array_count[dimension++] = dim;
        }
    };

    class scope : public RefCountedObject
    {
    public:
        scope(scope* outer, ast_node* owner = nullptr) noexcept
            : m_outer(outer), m_owner(owner) {}

        _NODISCARD_ scope* get_outer() const noexcept { return m_outer; }
        _NODISCARD_ ast_node* get_owner() const noexcept { return m_owner; }
        _NODISCARD_ ast_node* find_local_symbol(const String& name) const
        {
            auto it = m_symbols.find(name);
            if (it != m_symbols.end())
                return it->second.owner;
            return nullptr;
        }
        void push_symbol(const String& text, enum_symbol_type type, ast_node* node)
        {
            shader_lang_symbol symbol;
            symbol.name = text;
            symbol.type = type;
            symbol.owner = node;
            m_symbols[text] = symbol;
        }
		
    private:
        TRefCountPtr<scope> m_outer;
        ast_node* m_owner;
        TMap<String, shader_lang_symbol> m_symbols; // 当前作用域的符号表
    };

    using scope_ref = TRefCountPtr<scope>;

    struct parse_node
    {
        union
        {
            class ast_node* node;
            class ast_node_type* type;
            class ast_node_statement* statement;
            class ast_node_block* block;
            class ast_node_expression* expression;
        };

        token_data token;
        int op_type;
        dimensions dimension;
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
        type_qualifier_flags qualifiers = static_cast<type_qualifier_flags>(enum_type_qualifier::none); // 类型限定符
    };

    // 变量基类，包含参数变量，局部变量，全局变量，shader semantic等 // Definition
    class ast_node_variable : public ast_node
    {
    public:
        ast_node_variable(ast_node_type* var_type, String name)
            : ast_node(enum_ast_node_type::variable), type(var_type), name(std::move(name)) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    public:
        ast_node_type* type = nullptr;
        String name; // 用于存储变量名称
        dimensions dimension; // 用于存储变量的维度信息
        String semantic; // 用于存储shader语义
        String value; // 用于存储常量值或默认值
    };

    class ast_node_archive : public ast_node
    {
    public:
        ast_node_archive(const String& text) : ast_node(enum_ast_node_type::archive), name(text) {}

        scope* begin_archive();
        void end_archive(scope* current);

        void add_pass(class ast_node_pass* pass)
        {
            passes.push_back(pass);
        }
        void add_variant(class ast_node_variable* var)
        {
            variants.push_back(var);
        }
        void add_object_parameter(ast_node_variable* var)
        {
            object_parameters.push_back(var);
        }
        void add_pass_parameter(ast_node_variable* var)
        {
            pass_parameters.push_back(var);
        }
        void add_global_parameter(ast_node_variable* var)
        {
            global_parameters.push_back(var);
        }

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;

    private:
        String name;
        scope_ref global_scope;
        TArray<ast_node_variable*> properties;
        TArray<ast_node_variable*> variants;
        TArray<ast_node_variable*> object_parameters;
        TArray<ast_node_variable*> pass_parameters;
        TArray<ast_node_variable*> global_parameters;
        TArray<ast_node_pass*> passes;
        
    };

    class ast_node_pass : public ast_node
    {
    public:
        ast_node_pass() : ast_node(enum_ast_node_type::pass) {}
        void add_structure(class ast_node_struct* structure)
        {
            structures.push_back(structure);
        }
        void add_function(class ast_node_function* function)
        {
            functions.push_back(function);
        }
        
        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    private:
        TArray<ast_node_struct*> structures;
        TArray<ast_node_function*> functions;
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

        scope* begin_structure(scope* outer);
        void end_structure(scope* current);

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
        void add_member(ast_node_variable* mem)
        {
            members.push_back(mem);
            local_scope->push_symbol(mem->name, enum_symbol_type::variable, mem);
        }
    private:
        scope_ref local_scope; // 作用域引用，用于符号表管理
        ast_node_type* type = nullptr;
        TArray<ast_node_variable*> members;
    };

    class ast_node_function : public ast_node
    {
    public:
        ast_node_function(const String& name)
        : ast_node(enum_ast_node_type::function), func_name(name) {}

        void set_return_type(ast_node_type* ret_type)
        {
            return_type = ret_type;
        }
        void add_parameter(ast_node_variable* param)
        {
            params.push_back(param);
            local_scope->push_symbol(param->name, enum_symbol_type::variable, param);
        }
        void set_body(class ast_node_block* body_statements)
        {
            body = body_statements;
        }
        scope* begin_function(scope* outer);
        void end_function(scope* current);

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;

    private:
        scope_ref local_scope; // 作用域引用，用于符号表管理
        // func_signature
        ast_node_type* return_type = nullptr;
        NameHandle func_name = nullptr;
        TArray<ast_node_variable*> params;
        // body
        ast_node_block* body = nullptr;
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
        variable_declaration_statement(ast_node_variable* var, ast_node_expression* expr) noexcept
        : ast_node_statement(enum_statement_type::declare), variable(var), decl_expr(expr) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    private:
        ast_node_variable* variable = nullptr;
        ast_node_expression* decl_expr = nullptr;
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

    class break_statement : public ast_node_statement
    {
    public:
        break_statement() noexcept
        : ast_node_statement(enum_statement_type::break_) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    };

    class continue_statement : public ast_node_statement
    {
    public:
        continue_statement() noexcept
        : ast_node_statement(enum_statement_type::continue_) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    };

    class discard_statement : public ast_node_statement
    {
    public:
        discard_statement() noexcept
        : ast_node_statement(enum_statement_type::discard_) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    };

    class expression_statement : public ast_node_statement
    {
    public:
        expression_statement(ast_node_expression* expr) noexcept
        : ast_node_statement(enum_statement_type::expression), expr(expr) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    private:
        ast_node_expression* expr = nullptr;
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

    class for_statement : public ast_node_statement
    {
    public:
        for_statement(ast_node_statement* init, ast_node_expression* cond, ast_node_expression* update, ast_node_statement* body) noexcept
        : ast_node_statement(enum_statement_type::undefined), init_stmt(init), condition(cond), update_expr(update), body_stmt(body) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    public:
        ast_node_statement* init_stmt = nullptr;
        ast_node_expression* condition = nullptr;
        ast_node_expression* update_expr = nullptr;
        ast_node_statement* body_stmt = nullptr;
    };

    class ast_node_block : public ast_node_statement
    {
    public:
        ast_node_block() : ast_node_statement(enum_statement_type::block) {}
        void add_statement(ast_node_statement* statement)
        {
            statements.push_back(statement);
        }

        scope* begin_block(scope* outer);
        void end_block(scope* current);
        
        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    private:
        TArray<ast_node_statement*> statements; // 存储语句列表
        scope_ref local_scope; // 作用域引用，用于符号表管理
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

    class binary_expression : public ast_node_expression
    {
    public:
        binary_expression() noexcept
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
        component_expression(ast_node_expression* expr, String text) noexcept
            : ast_node_expression(enum_expr_type::component),
            component_name(expr), identifier(std::move(text)) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    private:
        ast_node_expression* component_name = nullptr;
        String identifier;
    };

    class index_expression : public ast_node_expression
    {
    public:
        index_expression(ast_node_expression* expr, ast_node_expression* index_expr) noexcept
            : ast_node_expression(enum_expr_type::index), target(expr), index(index_expr) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
    private:
        ast_node_expression* target = nullptr; // 被索引的表达式
        ast_node_expression* index = nullptr; // 索引表达式
    };

    // 出现过的id
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

    class function_call_expression : public ast_node_expression
    {
    public:
        function_call_expression(String func_name) noexcept
            : ast_node_expression(enum_expr_type::function_call), function_name(std::move(func_name)) {}

        void add_argument(ast_node_expression* arg)
        {
            arguments.push_back(arg);
        }

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
        evaluate_expr_result evaluate() override;
    private:
        String function_name;
        TArray<ast_node_expression*> arguments = {};
    };

    class constructor_expression : public ast_node_expression
    {
    public:
        constructor_expression(ast_node_type* type) noexcept
            : ast_node_expression(enum_expr_type::constructor_call), constructor_type(type) {}

        void add_argument(ast_node_expression* arg)
        {
            arguments.push_back(arg);
        }

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
        evaluate_expr_result evaluate() override;
    private:
        ast_node_type* constructor_type = nullptr;
        TArray<ast_node_expression*> arguments = {};
    };

    class assignment_expression : public ast_node_expression
    {
    public:
        assignment_expression(ast_node_expression* lhs, ast_node_expression* rhs) noexcept
            : ast_node_expression(enum_expr_type::assignment), left_expr(lhs), right_expr(rhs) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
        evaluate_expr_result evaluate() override;
    private:
        ast_node_expression* left_expr = nullptr;
        ast_node_expression* right_expr = nullptr;
    };

    class conditional_expression : public ast_node_expression
    {
    public:
        conditional_expression(ast_node_expression* cond, ast_node_expression* true_expr, ast_node_expression* false_expr) noexcept
            : ast_node_expression(enum_expr_type::conditional), condition(cond), true_expression(true_expr), false_expression(false_expr) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
        evaluate_expr_result evaluate() override;
    private:
        ast_node_expression* condition = nullptr;
        ast_node_expression* true_expression = nullptr;
        ast_node_expression* false_expression = nullptr;
    };

    class unary_expression : public ast_node_expression
    {
    public:
        unary_expression(enum_unary_op op, ast_node_expression* expr) noexcept
            : ast_node_expression(enum_expr_type::unary_op), op(op), operand(expr) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
        evaluate_expr_result evaluate() override;
    public:
        enum_unary_op op = enum_unary_op::undefined;
        ast_node_expression* operand = nullptr;
    };

    class compound_assignment_expression : public ast_node_expression
    {
    public:
        compound_assignment_expression(enum_assignment_op op, ast_node_expression* lhs, ast_node_expression* rhs) noexcept
            : ast_node_expression(enum_expr_type::compound_assignment), op(op), left_expr(lhs), right_expr(rhs) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
        evaluate_expr_result evaluate() override;
    public:
        enum_assignment_op op = enum_assignment_op::undefined;
        ast_node_expression* left_expr = nullptr;
        ast_node_expression* right_expr = nullptr;
    };

    // 链式表达式（逗号表达式）
    class chain_expression : public ast_node_expression
    {
    public:
        chain_expression(ast_node_expression* first_expr) noexcept
            : ast_node_expression(enum_expr_type::chain)
        {
            expressions.push_back(first_expr);
        }

        void add_expression(ast_node_expression* expr)
        {
            expressions.push_back(expr);
        }

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
        evaluate_expr_result evaluate() override;

    private:
        TArray<ast_node_expression*> expressions;
    };

    // 类型转换表达式
    class cast_expression : public ast_node_expression
    {
    public:
        cast_expression(ast_node_type* target_type, ast_node_expression* expr) noexcept
            : ast_node_expression(enum_expr_type::cast), cast_type(target_type), operand(expr) {}

        void generate_hlsl(String& outResult) override;
        void print_ast(int indent) override;
        evaluate_expr_result evaluate() override;

    private:
        ast_node_type* cast_type = nullptr;
        ast_node_expression* operand = nullptr;
    };

    class custom_expression : public ast_node_expression
    {
        
    };

    //////////////////////////////////////////////////////////////////////////
    // Tokenize

    int tokenize(token_data& t, const parse_location* loc, const char* text, int text_len, int token);
}
