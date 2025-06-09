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

	void shader_lang_state::insert_symbol_table(const String& name, enum_symbol_type type)
	{
		if (reserved_word_list.contains(name))
		{
			// 错误处理：不能使用保留字作为符号名
			DebugLog(this, enum_debug_level::error, "Cannot use reserved word as symbol name:", 0);
			return;
		}
		if (symbol_table.contains(name))
		{
			// 错误处理：符号已存在
			DebugLog(this, enum_debug_level::error, "Symbol already exists: ", 0);
		}
		else
		{
			symbol_table[name] = { name, type };
		}
	}

	void shader_lang_state::validate_symbol(const String& name, enum_symbol_type type)
	{
		if (reserved_word_list.contains(name))
		{
			// 错误处理：不能使用保留字作为符号名
			DebugLog(this, enum_debug_level::error, "Cannot use reserved word as symbol name: ", 0);
			return;
		}
		if (!symbol_table.contains(name))
		{
			DebugLog(this, enum_debug_level::error, "Symbol not found: ", 0);
		}
	}

	void DebugLog(shader_lang_state* state, enum_debug_level debugLevel, const char* message, int lineNo)
	{
		printf("%s\n", message);
	}
}
