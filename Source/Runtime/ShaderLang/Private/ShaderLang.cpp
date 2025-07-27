#pragma optimize("", off)
#include "ShaderLang.h"

#include "Assertion.h"
#include "AstNode.h"
#include "../Generated/parser.tab.h"

#include "ShaderCompiler.h"
#include "Templates/RefCounting.h"

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

	shader_lang_state::~shader_lang_state()
	{
		if (current_location)
		{
			delete current_location;
			current_location = nullptr;
		}
	}

	// 作用域管理实现
	void shader_lang_state::push_scope(scope* in_scope)
	{
		symbol_scopes.push_back(in_scope);
	}

	scope* shader_lang_state::pop_scope()
	{
		scope* cur_scope = symbol_scopes.back();
		symbol_scopes.pop_back();
		return cur_scope;
	}

	scope* shader_lang_state::current_scope() const
	{
		return symbol_scopes.empty() ? nullptr : symbol_scopes.back();
	}

	ast_node* shader_lang_state::get_local_symbol(const String& name) const
	{
		if (current_scope())
		{
			return current_scope()->find_local_symbol(name);
		}
		return nullptr;
	}

	ast_node* shader_lang_state::get_global_symbol(const String& name) const
	{
		// 从最内层作用域向外层查找
		for (auto it = symbol_scopes.rbegin(); it != symbol_scopes.rend(); ++it)
		{
			ast_node* node = (*it)->find_local_symbol(name);
			if (node != nullptr)
			{
				return node;
			}
		}
		return nullptr;
	}

	enum_symbol_type shader_lang_state::get_symbol_type(const String& name) const
	{
		// 从最内层作用域向外层查找符号类型
		for (auto it = symbol_scopes.rbegin(); it != symbol_scopes.rend(); ++it)
		{
			ast_node* node = (*it)->find_local_symbol(name);
			if (node != nullptr)
			{
				// 根据AST节点类型推断符号类型
				switch (node->Type)
				{
				case enum_ast_node_type::type:
					return enum_symbol_type::type;
				case enum_ast_node_type::function:
					return enum_symbol_type::function;
				case enum_ast_node_type::variable:
				case enum_ast_node_type::statement:
					return enum_symbol_type::variable;
				default:
					break;
				}
			}
		}
		return enum_symbol_type::undefined;
	}

	ast_node_type* shader_lang_state::create_type_node(const token_data& type_info)
	{
		const auto node = new ast_node_type;
		node->type_name = type_info.text;

		switch (type_info.token_id)
		{
		case TYPE_TEXTURE:
			node->param_type = enum_basic_type::tp_texture;
			break;
		case TYPE_SAMPLER:
			node->param_type = enum_basic_type::tp_sampler;
			break;
		case TYPE_VECTOR:
			node->param_type = enum_basic_type::tp_vector;
			break;
		case TYPE_MATRIX:
			node->param_type = enum_basic_type::tp_matrix;
			break;
		case TYPE_INT:
			node->param_type = enum_basic_type::tp_int;
			break;
		case TYPE_FLOAT:
			node->param_type = enum_basic_type::tp_float;
			break;
		case TYPE_VOID:
			node->param_type = enum_basic_type::tp_void;
			break;
		case TYPE_ID:
			{
				const auto symbol_node = get_global_symbol(type_info.text);
				if (symbol_node->Type == enum_ast_node_type::type)
				{
					//node->param_type = static_cast<ast_node_type*>(symbol_node)->param_type;
				}
				else if (symbol_node->Type == enum_ast_node_type::structure)
				{
					node->param_type = enum_basic_type::tp_struct;
				}
				else
				{
					TAssert(false);
				}
				break;
			}
		default:
			break;
		}

		return node;
	}

	void shader_lang_state::parsing_archive_begin(const token_data& name)
	{
		current_archive = new ast_node_archive(name.text);
		push_scope(current_archive->begin_archive());
	}

	ast_node* shader_lang_state::parsing_archive_end(ast_node* content)
	{
		current_archive->end_archive(pop_scope());

		ast_node_archive* dummy = current_archive;
		current_archive = nullptr;
		return dummy;
	}

	void shader_lang_state::parsing_pass_begin()
	{
		TAssert(current_pass == nullptr);
		current_pass = new ast_node_pass;
	}

	ast_node* shader_lang_state::parsing_pass_end()
	{
		current_archive->add_pass(current_pass);
		ast_node_pass* dummy = current_pass;
		current_pass = nullptr;
		return dummy;
	}

	void shader_lang_state::add_definition_member(ast_node* def)
	{
		if (def != nullptr)
		{
			if (def->Type == enum_ast_node_type::structure)
			{
				current_pass->add_structure(static_cast<ast_node_struct*>(def));
			}
			else if (def->Type == enum_ast_node_type::function)
			{
				current_pass->add_function(static_cast<ast_node_function*>(def));
			}
			else
			{
				debug_log("Invalid definition type in add_definition_member.");
			}
		}
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
		current_structure = new ast_node_struct(type);
		current_scope()->push_symbol(name.text, enum_symbol_type::structure, current_structure);
		push_scope(current_structure->begin_structure(current_scope()));
	}

	ast_node_struct* shader_lang_state::parsing_struct_end()
	{
		current_structure->end_structure(pop_scope());

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

	void shader_lang_state::add_variant_member(ast_node* type, const token_data& name, ast_node_expression* default_value)
	{
		TAssertf(type != nullptr && type->Type == enum_ast_node_type::type,
			"add_variant_member called with invalid type.");
		TAssert(name.token_id == NEW_ID);

		const auto variable = new ast_node_variable(name.text);
		variable->type = static_cast<ast_node_type*>(type);
		
		if (default_value != nullptr)
		{
			// 这里可以将default_value转换为字符串存储到variable->value中
			// 具体实现取决于如何处理默认值
		}

		if (current_archive)
		{
			current_archive->add_variant(variable);
		}
		else
		{
			debug_log("No current variants to add member to.");
		}
	}

	void shader_lang_state::add_parameter_member(ast_node* type, const token_data& name, ast_node_expression* default_value)
	{
		TAssertf(type != nullptr && type->Type == enum_ast_node_type::type,
			"add_parameter_member called with invalid type.");
		TAssert(name.token_id == NEW_ID);

		const auto variable = new ast_node_variable(name.text);
		variable->type = static_cast<ast_node_type*>(type);
		
		if (default_value != nullptr)
		{
			// 这里可以将default_value转换为字符串存储到variable->value中
			// 具体实现取决于如何处理默认值
		}

		if (current_archive)
		{
			current_archive->add_pass_parameter(variable);
		}
		else
		{
			debug_log("No current parameters to add member to.");
		}
	}

	void shader_lang_state::parsing_function_begin(ast_node* type, const token_data& name)
	{
		TAssert(current_function == nullptr);
		TAssert(name.token_id == NEW_ID);
		current_function = new ast_node_function(name.text);
		current_function->set_return_type(static_cast<ast_node_type*>(type));
		current_scope()->push_symbol(name.text, enum_symbol_type::function, current_function);
		push_scope(current_function->begin_function(current_scope()));
	}

	ast_node_function* shader_lang_state::parsing_function_end(ast_node_block* body)
	{
		TAssert(current_function != nullptr);
		current_function->set_body(body);
		current_function->end_function(pop_scope());

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
		auto* new_block = new ast_node_block;
		push_scope(new_block->begin_block(current_scope()));
		block_stacks.push_back(new_block);
	}

	ast_node_block* shader_lang_state::parsing_block_end()
	{
		const auto current_block = block_stacks.back();
		current_block->end_block(pop_scope());
		block_stacks.pop_back();
		
		return current_block;
	}

	void shader_lang_state::add_block_statement(ast_node_statement* statement, const parse_location* loc) const
	{
		if (!block_stacks.empty())
		{
			block_stacks.back()->add_statement(statement);
		}
		else
		{
			debug_log("No current block to add statement to.", loc);
		}
	}

	ast_node_statement* shader_lang_state::create_var_decl_statement(
		ast_node_type* type_node, const token_data& name, ast_node_expression* init_expr)
	{
		TAssert(name.token_id == NEW_ID);
		const auto node = new variable_declaration_statement(type_node, name.text, init_expr);

		current_scope()->push_symbol(name.text, enum_symbol_type::variable, node);
		return node;
	}

	ast_node_statement* shader_lang_state::create_return_statement(ast_node_expression* expr)
	{
		return new return_statement(expr);
	}

	ast_node_statement* shader_lang_state::create_expression_statement(ast_node_expression* expr)
	{
		return new expression_statement(expr);
	}

	ast_node_statement* shader_lang_state::create_condition_statement(
		ast_node_expression* cond, ast_node_statement* true_stmt, ast_node_statement* false_stmt)
	{
		return new condition_statement(cond, true_stmt, false_stmt);
	}

	ast_node_statement* shader_lang_state::create_for_statement(
		ast_node_statement* init, ast_node_expression* cond, ast_node_expression* update, ast_node_statement* body)
	{
		return new for_statement(init, cond, update, body);
	}

	ast_node_expression* shader_lang_state::create_function_call_expression(const token_data& func_name)
	{
		return new function_call_expression(func_name.text);
	}

	void shader_lang_state::append_argument(ast_node_expression* func_call_expr, ast_node_expression* arg_expr)
	{
		TAssert(func_call_expr != nullptr && func_call_expr->expr_type == enum_expr_type::function_call);
		const auto expr = static_cast<function_call_expression*>(func_call_expr);
		expr->add_argument(arg_expr);
	}

	ast_node_expression* shader_lang_state::create_assignment_expression(ast_node_expression* lhs, ast_node_expression* rhs)
	{
		return new assignment_expression(lhs, rhs);
	}

	ast_node_expression* shader_lang_state::create_conditional_expression(ast_node_expression* cond, ast_node_expression* true_expr, ast_node_expression* false_expr)
	{
		return new conditional_expression(cond, true_expr, false_expr);
	}

	ast_node_expression* shader_lang_state::create_reference_expression(const token_data& name)
	{
		const auto ref_expr = new reference_expression(name.text, get_global_symbol(name.text));
		return ref_expr;
	}

	ast_node_expression* shader_lang_state::create_binary_expression(
		enum_binary_op op, ast_node_expression* left, ast_node_expression* right)
	{
		const auto node = new binary_expression;
		node->op = op;
		node->left = left;
		node->right = right;
		node->expr_data_type = node->left->expr_data_type;
		//TAssert(owner->left->expr_data_type == owner->right->expr_data_type);
		return node;
	}

	ast_node_expression* shader_lang_state::create_unary_expression(int op, ast_node_expression* operand)
	{
		return new unary_expression(static_cast<enum_unary_op>(op), operand);
	}

	ast_node_expression* shader_lang_state::create_compound_assignment_expression(int op, ast_node_expression* lhs, ast_node_expression* rhs)
	{
		return new compound_assignment_expression(static_cast<enum_assignment_op>(op), lhs, rhs);
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
			/*const auto symbol_node = get_global_symbol(comp.text);
			if(symbol_node == nullptr)
			{
				debug_log("Symbol not found: " + comp.text);
				return nullptr;
			}*/

			const auto component = new component_expression(expr, comp.text);
			return component;
		}
	}

	// 常量表达式创建函数
	ast_node_expression* shader_lang_state::create_constant_int_expression(int value)
	{
		return new constant_int_expression(value);
	}

	ast_node_expression* shader_lang_state::create_constant_float_expression(float value)
	{
		return new constant_float_expression(value);
	}

	ast_node_expression* shader_lang_state::create_constant_bool_expression(bool value)
	{
		return new constant_bool_expression(value);
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

	void shader_lang_state::post_process_ast() const
	{
		if (ast_root == nullptr)
		{
			debug_log("Parse Error, current text : " + sl_state->current_text);
			return;
		}
		ast_root->print_ast(0);
		printf("\n");
		String outHlsl;
		ast_root->generate_hlsl(outHlsl);
		printf("-------generate hlsl-------\n%s", outHlsl.c_str());

		printf("\n-------DXCompiler-------\n");
		BinaryData ByteCode;
		const TRefCountPtr<ICompiler> ShaderCompiler = new DXCCompiler();
		ShaderCompiler->Compile(nullptr, outHlsl, outHlsl.size(), {}, "", "main", "ps_6_0", ByteCode);
		if (ByteCode.Data != nullptr)
		{
			printf("ByteCode Size: %d\n", static_cast<int32>(ByteCode.Size));
			TMemory::Destroy(ByteCode.Data);
		}
		else
		{
			printf("ByteCode is null\n");
		}
	}



}
#pragma optimize("", on)