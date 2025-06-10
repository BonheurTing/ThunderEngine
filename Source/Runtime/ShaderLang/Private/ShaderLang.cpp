#include "ShaderLang.h"
#include "AstNode.h"

namespace Thunder
{
	TSet<String> reserved_word_list = {
		"int", "float", "void", "return", "if", "else", "for", "while",
		"break", "continue", "switch", "case", "default", "do", "struct",
		"union", "enum", "typedef", "const", "static", "extern",
		"inline", "sizeof"
	};

	void shader_lang_state::insert_symbol_table(const String& name, enum_symbol_type type, const parse_location* loc)
	{
		if (reserved_word_list.contains(name))
		{
			// 错误处理：不能使用保留字作为符号名
			debug_log("Cannot use reserved word as symbol name:" + name, loc);
			return;
		}
		if (symbol_table.contains(name))
		{
			// 错误处理：符号已存在
			debug_log("Symbol already exists: " + name, loc);
		}
		else
		{
			symbol_table[name] = { name, type };
		}
	}

	void shader_lang_state::evaluate_symbol(const String& name, enum_symbol_type type, const parse_location* loc) const
	{
		if (reserved_word_list.contains(name))
		{
			// 错误处理：不能使用保留字作为符号名
			debug_log("Cannot use reserved word as symbol name: " + name, loc);
			return;
		}
		if (!symbol_table.contains(name))
		{
			debug_log("Symbol not found: " + name, loc);
		}
	}

	const char* get_file(int file_id)
	{
		return "";
	}

	void shader_lang_state::debug_log(const String& msg, const parse_location* loc)
	{
		printf("Parsing error with <%s> : line %d col %d  Message : %s\n",
			get_file(loc->first_source), loc->first_line, loc->first_column, msg.c_str());
	}
}
