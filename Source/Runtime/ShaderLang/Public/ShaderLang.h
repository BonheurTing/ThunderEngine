#pragma once

#include "Container.h"
#include "Platform.h"
#include "AstNode.h"

namespace Thunder
{
	// 调试信息条目
	struct debug_info
	{
		String message;
		int line_no;
		debug_info* next;
	};

	// 调试信息级别
	enum class enum_debug_level : uint8
	{
		none,
		info,
		verbose,
		debug,
		warning,
		error,
		all
	};
	
	/* Shader parsing state.
	 * 核心功能：
	* 1，符号表管理（变量、函数、类型等）
	* 2，错误处理与恢复（记录错误位置和类型）
	* 3，作用域管理（全局作用域、函数作用域、块作用域）
	* 4，抽象语法树（AST）构建
	* 5，文件包含/导入管理（处理多文件解析）
	* 6，类型系统状态（类型推断、类型检查）
	* 7，调试信息收集（行号、列号、文件名跟踪）
	 */
	struct shader_lang_state
	{
		shader_lang_state();
		~shader_lang_state();

		// 调试信息
		String current_text; // 当前token文本
		parse_location* current_location = nullptr; // 当前解析位置
		
		// 词法/语法分析器状态
		void *scanner = nullptr;	// Flex词法分析器上下文
		//void *parser = nullptr;		// Bison语法分析器状态

		/* 抽象语法树 */
		ast_node* ast_root = nullptr;	// AST根节点
		scope_ref global_scope = nullptr;	// 全局作用域

		// 作用域
		TArray<scope*> symbol_scopes;

		// function stacks.
		//std::vector<ast_node_function*> functions;
		// block stacks.
		TArray<ast_node_block*> block_stacks;

		/* 状态机运行时上下文 */
		ast_node_pass* current_pass = nullptr;
		ast_node_struct* current_structure = nullptr;
		ast_node_function* current_function = nullptr;
		ast_node_archive* current_archive = nullptr;

		/* 上下文管理 */
		ast_node_type* create_type_node(const token_data& type_info);

		void parsing_archive_begin(const token_data& name);
		ast_node* parsing_archive_end(ast_node* content);
		void add_variant_member(ast_node* type, const token_data& name, ast_node_expression* default_value);
		void add_parameter_member(ast_node* type, const token_data& name, ast_node_expression* default_value);

		void parsing_pass_begin();
		ast_node* parsing_pass_end();
		void add_definition_member(ast_node* def);
		
		void parsing_struct_begin(const token_data& name);
		ast_node_struct* parsing_struct_end();
		void add_struct_member(ast_node* type, const token_data& name, const struct token_data& modifier, const parse_location* loc);
		static void bind_modifier(ast_node* type, const token_data& modifier, const parse_location* loc);
		
		void parsing_function_begin(ast_node* type, const token_data& name);
		ast_node_function* parsing_function_end(ast_node_block* body);
		void add_function_param(ast_node* type, const token_data& name, const parse_location* loc);

		void parsing_block_begin();
		ast_node_block* parsing_block_end();
		void add_block_statement(ast_node_statement* statement, const parse_location* loc) const;

		/* Statement */
		ast_node_statement* create_var_decl_statement(ast_node_type* type_node, const token_data& name, ast_node_expression* init_expr);
		ast_node_statement* create_return_statement(ast_node_expression* expr);
		ast_node_statement* create_expression_statement(ast_node_expression* expr);
		ast_node_statement* create_condition_statement(ast_node_expression* cond, ast_node_statement* true_stmt, ast_node_statement* false_stmt);
		ast_node_statement* create_for_statement(ast_node_statement* init, ast_node_expression* cond, ast_node_expression* update, ast_node_statement* body);

		/* Expression */
		ast_node_expression* create_reference_expression(const token_data& name);
		ast_node_expression* create_binary_expression(enum_binary_op op, ast_node_expression* left, ast_node_expression* right);
		ast_node_expression* create_unary_expression(int op, ast_node_expression* operand);
		ast_node_expression* create_compound_assignment_expression(int op, ast_node_expression* lhs, ast_node_expression* rhs);
		ast_node_expression* create_shuffle_or_component_expression(ast_node_expression* expr, const token_data& comp);
		ast_node_expression* create_function_call_expression(const token_data& func_name);
		void append_argument(ast_node_expression* func_call_expr, ast_node_expression* arg_expr);
		ast_node_expression* create_assignment_expression(ast_node_expression* lhs, ast_node_expression* rhs);
		ast_node_expression* create_conditional_expression(ast_node_expression* cond, ast_node_expression* true_expr, ast_node_expression* false_expr);
		
		/* Constant Expression */
		ast_node_expression* create_constant_int_expression(int value);
		ast_node_expression* create_constant_float_expression(float value);
		ast_node_expression* create_constant_bool_expression(bool value);

		/* 作用域 & 符号表 管理 */
		void push_scope(scope* in_scope);
		scope* pop_scope();
		scope* current_scope() const;
		ast_node* get_local_symbol(const String& name) const;
		ast_node* get_global_symbol(const String& name) const;
		enum_symbol_type get_symbol_type(const String& name) const;

		/* 类型系统 */
		//DataType* type_infer(ShaderParseState* state, ASTNode* expr) { return nullptr;}    // 类型推断

		/* 错误处理 */
		void debug_log(const String& msg, const parse_location* loc = nullptr) const;
		void post_process_ast() const;

	};
	extern shader_lang_state* sl_state;
}
