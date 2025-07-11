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
		TAssert(current_structure == nullptr);
		TAssert(name.token_id == NEW_ID);
		const auto type = create_type_node(name);
		if (const auto struct_type = static_cast<ast_node_type*>(type))
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
		TAssert(current_function == nullptr);
		TAssert(name.token_id == NEW_ID);
		current_function = new ast_node_function(name.text);
		insert_symbol_table(name, enum_symbol_type::function, current_function);
		current_function->set_return_type(static_cast<ast_node_type*>(type));
	}

	ast_node_function* shader_lang_state::parsing_function_end(ast_node_block* body)
	{
		TAssert(current_function != nullptr);
		current_function->set_body(body);
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

	void shader_lang_state::parsing_block_begin()
	{
		TAssert(current_block == nullptr);
		current_block = new ast_node_block;
	}

	ast_node_block* shader_lang_state::parsing_block_end()
	{
		ast_node_block* dummy = current_block;
		current_block = nullptr;
		return dummy;
	}

	void shader_lang_state::add_block_statement(ast_node_statement* statement, const parse_location* loc) const
	{
		if (current_block)
		{
			current_block->add_statement(statement);
		}
		else
		{
			debug_log("No current block to add statement to.", loc);
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

	ast_node_statement* shader_lang_state::create_var_decl_statement(
		ast_node* typeNode, const token_data& name, ast_node* init_expr)
	{
		TAssertf(typeNode != nullptr && typeNode->Type == enum_ast_node_type::type, "Type owner type is not correct");
		TAssert(name.token_id == NEW_ID);
		const auto node = new ast_node_var_declaration;
		node->VarDelType = static_cast<ast_node_type*>(typeNode);
		node->VarName = name.text;

		if (init_expr != nullptr)
		{
			TAssertf(init_expr->Type == enum_ast_node_type::expression, "Init expression owner type is not correct");
			node->DelExpression = static_cast<ast_node_expression*>(init_expr);
		}

		sl_state->insert_symbol_table(name, enum_symbol_type::variable, node);
		return node;
	}

	ast_node_statement* shader_lang_state::create_assignment_statement(const token_data& lhs, ast_node* rhs)
	{
		TAssertf(rhs != nullptr && rhs->Type == enum_ast_node_type::expression, "Assignment rhs is null");
		TAssert(lhs.token_id == TOKEN_IDENTIFIER);
		const auto node = new ast_node_assignment;
		node->LhsVar = lhs.text;
		node->RhsExpression = static_cast<ast_node_expression*>(rhs);
		return node;
	}

	ast_node_statement* shader_lang_state::create_return_statement(ast_node* expr)
	{
		TAssertf(expr != nullptr && expr->Type == enum_ast_node_type::expression, "Return expression is null");
		const auto node = new ast_node_return;
		node->RetValue = static_cast<ast_node_expression*>(expr);
		return node;
	}
}
#pragma optimize("", on)