#pragma optimize("", off)
#include "ShaderLang.h"

#include "Assertion.h"
#include "AstNode.h"
#include "../Generated/parser.tab.h"

namespace Thunder
{
	TSet<String> reserved_word_list = {
		"int", "float", "void", "return", "if", "else", "for", "while",
		"break", "continue", "switch", "case", "default", "do", "struct",
		"union", "enum", "typedef", "const", "static", "extern",
		"inline", "sizeof"
	};

	shader_lang_state::shader_lang_state()
	{
		current_location = new parse_location(0, 0, 0, 0, 0, 0);
		current_structure = nullptr;
		current_function = nullptr;
	}

	void shader_lang_state::parsing_struct_begin(const token_data& name)
	{
		TAssert(name.token_id == NEW_ID);
		const auto type = create_type_node(name);
		if (auto struct_type = static_cast<ast_node_type*>(type))
		{
			struct_type->param_type = enum_basic_type::tp_struct;
		}
		insert_symbol_table(name, enum_symbol_type::type, type);
		current_structure = new ast_node_struct(type);
	}

	ast_node_struct* shader_lang_state::parsing_struct_end()
	{
		ast_node_struct* dummy = current_structure;
		current_structure = nullptr;
		return dummy;
	}

	void shader_lang_state::add_struct_member(ast_node* type, const token_data& name, const token_data& modifier, const parse_location* loc)
	{
		TAssertf(type != nullptr && type->Type == enum_ast_node_type::type,
			"add_struct_member called with invalid type owner.");
		TAssert(name.token_id == NEW_ID);
		

		const auto variable = new ast_node_variable(name.text);
		variable->type = static_cast<ast_node_type*>(type);
		if(variable->type->is_semantic)
		{
			variable->semantic = modifier.text;
		}
		insert_symbol_table(name, enum_symbol_type::variable, variable);

		if (current_structure)
		{
			current_structure->add_member(variable);
		}
		else
		{
			debug_log("No current structure to add member to.", loc);
		}
	}

	void shader_lang_state::bind_modifier(ast_node* type, const token_data& modifier, const parse_location* loc)
	{
		TAssertf(type != nullptr && type->Type == enum_ast_node_type::type,
			"bind_modifier called with invalid type owner.");

		if (const auto type_node = static_cast<ast_node_type*>(type))
		{
			if (modifier.token_id == TOKEN_SV)
			{
				type_node->is_semantic = true;
			}
		}
	}

	void shader_lang_state::parsing_function_begin(ast_node* type, const token_data& name)
	{
		TAssert(name.token_id == NEW_ID);
		current_function = new ast_node_function(name.text);
		insert_symbol_table(name, enum_symbol_type::function, current_function);
		current_function->set_return_type(static_cast<ast_node_type*>(type));
	}

	ast_node_function* shader_lang_state::parsing_function_end()
	{
		ast_node_function* dummy = current_function;
		current_function = nullptr;
		return dummy;
	}

	void shader_lang_state::add_function_param(ast_node* type, const token_data& name, const parse_location* loc)
	{
		TAssertf(type != nullptr && type->Type == enum_ast_node_type::type,
			"add_struct_member called with invalid type owner.");
		TAssert(name.token_id == NEW_ID);

		const auto variable = new ast_node_variable(name.text);
		variable->type = static_cast<ast_node_type*>(type);
		insert_symbol_table(name, enum_symbol_type::variable, variable);
		
		if (current_function)
		{
			current_function->add_parameter(variable);
		}
		else
		{
			debug_log("No current function to add parameter to.", loc);
		}
	}

	void shader_lang_state::set_function_body(ast_node* body, const parse_location* loc)
	{
		TAssertf(body != nullptr && body->Type == enum_ast_node_type::statement_list,
			"set_function_body called with invalid statement list owner.");
		if (current_function)
		{
			current_function->set_body(static_cast<ast_node_statement_list*>(body));
		}
		else
		{
			debug_log("No current function to set body for.", loc);
		}
	}

	ast_node_expression* shader_lang_state::create_reference_expression(const token_data& name)
	{
		const auto ref_expr = new reference_expression(name.text, get_symbol_node(name.text));
		return ref_expr;
	}

	ast_node_expression* shader_lang_state::create_binary_op_expression(
		enum_binary_op op, ast_node_expression* left, ast_node_expression* right)
	{
		const auto node = new binary_op_expression;
		node->op = op;
		node->left = left;
		node->right = right;
		node->expr_data_type = node->left->expr_data_type;
		//TAssert(owner->left->expr_data_type == owner->right->expr_data_type);
		return node;
	}

	ast_node_expression* shader_lang_state::create_shuffle_or_component_expression(
		ast_node_expression* expr, const token_data& comp)
	{
		bool is_shuffle = true;
		char order[4] = { 0, 0, 0, 0 };
		if (comp.length <= 4)
		{
			for (int i = 0; i < comp.length; ++i)
			{
				switch (comp.text[i])
				{
				case 'r':
				case 'g':
				case 'b':
				case 'a':
				case 'x':
				case 'y':
				case 'z':
				case 'w':
					order[i] = comp.text[i];
					continue;
				}
				is_shuffle = false;
				break;
			}
		}
		else
		{
			is_shuffle = false;
		}

		if (is_shuffle)
		{
			auto* prev = new shuffle_expression(expr, order);
			return prev;
		}
		else
		{
			const auto symbol_node = sl_state->get_symbol_node(comp.text);
			if(symbol_node == nullptr)
			{
				sl_state->debug_log("Symbol not found: " + comp.text);
				return nullptr;
			}

			const auto component= new component_expression(expr, symbol_node, comp.text);
			return component;
		}
	}

	void shader_lang_state::insert_symbol_table(const token_data& sym, enum_symbol_type type, ast_node* node)
	{
		const auto loc = new parse_location(sym.first_line, sym.first_column, sym.first_source, sym.last_line, sym.last_column, sym.last_source);
		if (reserved_word_list.contains(sym.text))
		{
			// 错误处理：不能使用保留字作为符号名
			debug_log("Cannot use reserved word as symbol name:" + sym.text, loc);
			return;
		}
		if (symbol_table.contains(sym.text))
		{
			// 错误处理：符号已存在
			debug_log("Symbol already exists: " + sym.text, loc);
		}
		else
		{
			symbol_table[sym.text] = { sym.text, type, node };
		}
	}

	void shader_lang_state::evaluate_symbol(const token_data& name, enum_symbol_type type) const
	{
		TAssert(name.token_id == TOKEN_IDENTIFIER);
		const auto loc = new parse_location(name.first_line, name.first_column, name.first_source, name.last_line, name.last_column, name.last_source);
		if (reserved_word_list.contains(name.text))
		{
			// 错误处理：不能使用保留字作为符号名
			debug_log("Cannot use reserved word as symbol name: " + name.text, loc);
			return;
		}
		if (!symbol_table.contains(name.text))
		{
			debug_log("Symbol not found: " + name.text, loc);
		}
	}

	ast_node* shader_lang_state::get_symbol_node(const String& name)
	{
		if (symbol_table.contains(name))
		{
			return symbol_table[name].owner;
		}
		return nullptr;
	}

	enum_symbol_type shader_lang_state::get_symbol_type(const String& name)
	{
		if (symbol_table.contains(name))
		{
			return symbol_table[name].type;
		}
		return enum_symbol_type::undefined; // 未知类型
	}

	//todo 定位文件
	const char* get_file(int file_id) 
	{
		return "";
	}

	void shader_lang_state::debug_log(const String& msg, const parse_location* loc) const
	{
		if (loc == nullptr)
			loc = current_location;

		printf("Message : %s : line %d col %d \n", msg.c_str(), loc->first_line, loc->first_column);
	}
}
#pragma optimize("", on)