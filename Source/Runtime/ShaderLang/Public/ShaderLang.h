#pragma once

#include "Container.h"
#include "Platform.h"

namespace Thunder
{
	
	enum class basal_ast_node_type : uint8;

	enum class enum_symbol_type : uint8
	{
		variable,   // 变量
		function,   // 函数
		type,       // 类型
		constant,   // 常量
		undefined  // 未定义符号
	};

	struct shader_lang_symbol
	{
		String Name;
		enum_symbol_type Type;
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
		// 词法/语法分析器状态
		void *scanner = nullptr;	// Flex词法分析器上下文
		//void *parser = nullptr;		// Bison语法分析器状态

		// 符号表
		TMap<String, shader_lang_symbol> symbol_table = {};
		
		/* 抽象语法树 */
		class ast_node* ast_root = nullptr;	// AST根节点

		/* 符号表管理 */
		void insert_symbol_table(const String& name, enum_symbol_type type, const parse_location* loc);
		void evaluate_symbol(const String& name, enum_symbol_type type, const parse_location* loc) const;

		/* 作用域管理 */

		/* 类型系统 */
		//DataType* type_infer(ShaderParseState* state, ASTNode* expr) { return nullptr;}    // 类型推断

		/* 错误处理 */
		static void debug_log(const String& msg, const parse_location* loc);

	};

	
	
}