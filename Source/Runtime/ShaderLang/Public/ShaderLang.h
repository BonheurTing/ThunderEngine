#pragma once

#include "Container.h"
#include "Platform.h"
#include "AstNode.h"

namespace Thunder
{
	
	enum class enum_ast_node_type : uint8;

	enum class enum_symbol_type : uint8
	{
		variable,   // 变量 ast_node_var_declaration
		type,      // 类型 ast_node_type
		//structure,  // 结构体
		function,   // 函数 ast_node_function
		//constant,   // 常量
		undefined  // 未定义符号
	};

	struct shader_lang_symbol
	{
		String name;
		enum_symbol_type type = enum_symbol_type::undefined;
		ast_node* node = nullptr; //symbol所在的语法树节点
	};

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

		// 调试信息
		String current_text; // 当前token文本
		parse_location* current_location = nullptr; // 当前解析位置
		
		// 词法/语法分析器状态
		void *scanner = nullptr;	// Flex词法分析器上下文
		//void *parser = nullptr;		// Bison语法分析器状态

		// 符号表
		TMap<String, shader_lang_symbol> symbol_table = {};
		
		/* 抽象语法树 */
		ast_node* ast_root = nullptr;	// AST根节点

		/* 状态机运行时上下文 */
		ast_node_struct* current_structure = nullptr;
		ast_node_function* current_function = nullptr;

		/* 上下文管理 */
		void parsing_struct_begin(const token_data& name);
		ast_node_struct* parsing_struct_end();
		void add_struct_member(ast_node* type, const token_data& name, const struct token_data& modifier, const parse_location* loc);
		void bind_modifier(ast_node* type, const token_data& modifier, const parse_location* loc);
		void parsing_function_begin(ast_node* type, const token_data& name);
		ast_node_function* parsing_function_end();
		void add_function_param(ast_node* type, const token_data& name, const parse_location* loc);
		void set_function_body(ast_node* body, const parse_location* loc);

		/* 符号表管理 */
		void insert_symbol_table(const token_data& sym, enum_symbol_type type, ast_node* node);
		void evaluate_symbol(const token_data&  name, enum_symbol_type type) const;
		ast_node* get_symbol_node(const String& name);

		void TestIdentifier(const token_data& sym);
		void TestPrimitiveType(const token_data& sym);

		/* 作用域管理 */

		/* 类型系统 */
		//DataType* type_infer(ShaderParseState* state, ASTNode* expr) { return nullptr;}    // 类型推断

		/* 错误处理 */
		void debug_log(const String& msg, const parse_location* loc = nullptr) const;

	};
	extern shader_lang_state* sl_state;
}