#include "ShaderLang.h"

#include "Assertion.h"
#include "AstNode.h"

namespace Thunder
{
	TSet<String> reserved_word_list = {
		"int", "float", "void", "return", "if", "else", "for", "while",
		"break", "continue", "switch", "case", "default", "do", "struct",
		"union", "enum", "typedef", "const", "static", "extern",
		"inline", "sizeof"
	};

	void shader_lang_state::parsing_struct_begin(const String& name, const parse_location* loc)
	{
		insert_symbol_table(name, enum_symbol_type::structure, loc);
		current_structure = new ast_node_struct(name);
	}

	ast_node_struct* shader_lang_state::parsing_struct_end()
	{
		ast_node_struct* dummy = current_structure;
		current_structure = nullptr;
		return dummy;
	}

	void shader_lang_state::add_struct_member(ast_node* type, const String& name, const String& modifier, const parse_location* loc)
	{
		TAssertf(type != nullptr && type->Type == enum_ast_node_type::type,
			"add_struct_member called with invalid type node.");
		
		insert_symbol_table(name, enum_symbol_type::variable, loc);

		const auto variable = new ast_node_variable(name);
		variable->type = static_cast<ast_node_type*>(type);
		if(variable->type->is_semantic)
		{
			variable->semantic = modifier;
		}

		if (current_structure)
		{
			current_structure->add_member(variable);
		}
		else
		{
			debug_log("No current structure to add member to.", loc);
		}
	}

	void shader_lang_state::bind_modifier(ast_node* type, const String& modifier, const parse_location* loc)
	{
		TAssertf(type != nullptr && type->Type == enum_ast_node_type::type,
			"bind_modifier called with invalid type node.");

		if (const auto type_node = static_cast<ast_node_type*>(type))
		{
			type_node->is_semantic = true;
		}
	}

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
