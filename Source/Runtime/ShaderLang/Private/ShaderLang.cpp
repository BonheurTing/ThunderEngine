#pragma optimize("", off)
#include "ShaderLang.h"
#include "Assertion.h"
#include "AstNode.h"
#include "PreProcessor.h"
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

	ast_node* shader_lang_state::get_global_symbol(const String& name)
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
				switch (node->node_type)
				{
				case enum_ast_node_type::type_format:
					return enum_symbol_type::type_format;
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

	void shader_lang_state::add_variable_to_list(ast_node_type_format* type, const token_data& name, ast_node_expression* default_value)
	{
		TAssert(name.token_id == NEW_ID);

		const auto variable = new ast_node_variable(type, name.text);
		
		if (default_value != nullptr)
		{
			// 这里可以将default_value转换为字符串存储到variable->value中
			// 具体实现取决于如何处理默认值
		}

		current_variables.push_back(variable);
	}

	void shader_lang_state::parsing_variable_end(const token_data& name, const token_data& text)
	{
		if (name.token_id == TOKEN_VARIANTS)
		{
			for (const auto& var : current_variables)
			{
				current_archive->add_variant(var);
			}
		}
		else if (name.token_id == TOKEN_PARAMETERS)
		{
			if (text.text == "\"Object\"")
			{
				for (const auto& var : current_variables)
				{
					current_archive->add_object_parameter(var);
				}
			}
			else if (text.text == "\"Pass\"")
			{
				for (const auto& var : current_variables)
				{
					current_archive->add_pass_parameter(var);
				}
			}
			else if (text.text == "\"Global\"")
			{
				for (const auto& var : current_variables)
				{
					current_archive->add_global_parameter(var);
				}
			}
			else
			{
				debug_log("Unknown parameter type: " + text.text);
			}
		}
		else
		{
			debug_log("Unknown variable type: " + name.text);
		}
		current_variables.clear();
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
			if (def->node_type == enum_ast_node_type::structure)
			{
				current_pass->add_structure(static_cast<ast_node_struct*>(def));
			}
			else if (def->node_type == enum_ast_node_type::function)
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
		const auto type = new ast_node_type_format();
		type->basic_type = enum_basic_type::tp_struct;
		//type->type_text = name.text;
		type->indirect_index = static_cast<uint16>(custom_types.size());

		current_structure = new ast_node_struct(type, name.text);
		custom_types.push_back(current_structure);
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

	void shader_lang_state::add_struct_member(ast_node_type_format* type, const token_data& name, const token_data& modifier, const parse_location* loc)
	{
		TAssert(name.token_id == NEW_ID);
		const auto variable = new ast_node_variable(type, name.text);
		if(type->is_semantic)
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

	void shader_lang_state::parsing_function_begin(ast_node_type_format* type, const token_data& name)
	{
		TAssert(current_function == nullptr);
		TAssert(name.token_id == NEW_ID);
		current_function = new ast_node_function(name.text);
		current_function->set_return_type(type);
		current_scope()->push_symbol(name.text, enum_symbol_type::function, current_function);
		push_scope(current_function->begin_function(current_scope()));
	}

	ast_node_function* shader_lang_state::parsing_function_end(const token_data& info, ast_node_block* body)
	{
		TAssert(current_function != nullptr);
		if (info.token_id == TOKEN_SV)
		{
			current_function->set_semantic(info.text);
		}
		current_function->set_body(body);
		current_function->end_function(pop_scope());

		ast_node_function* dummy = current_function;
		current_function = nullptr;
		return dummy;
	}

	void shader_lang_state::add_function_param(ast_node_type_format* type, const token_data& name, const parse_location* loc)
	{
		TAssert(name.token_id == NEW_ID);
		const auto variable = new ast_node_variable(type, name.text);
		
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

	int shader_lang_state::evaluate_integer_expression(ast_node_expression* expr, const parse_location& loc)
	{
		const auto ret = expr->evaluate();
		if (ret.result_type == enum_eval_result_type::constant_int)
		{
			return ret.int_value;
		}
		else
		{
			String type_text;
			ret.type->generate_hlsl(type_text);
			debug_log("Expected integer expression, but " + type_text, &loc);
			return 0; // 或者抛出异常
		}
	}

	
	// 根据对象类型名称解析对象类型枚举
	enum_object_type parse_object_type_from_name(const String& type_name)
	{
		LOG(type_name.c_str());
		if (type_name == "Buffer") return enum_object_type::buffer;
		if (type_name == "ByteAddressBuffer") return enum_object_type::byte_address_buffer;
		if (type_name == "StructuredBuffer") return enum_object_type::structured_buffer;
		if (type_name == "AppendStructuredBuffer") return enum_object_type::append_structured_buffer;
		if (type_name == "ConsumeStructuredBuffer") return enum_object_type::consume_structured_buffer;
		if (type_name == "RWBuffer") return enum_object_type::rw_buffer;
		if (type_name == "RWByteAddressBuffer") return enum_object_type::rw_byte_address_buffer;
		if (type_name == "RWStructuredBuffer") return enum_object_type::rw_structured_buffer;
		
		if (type_name == "Texture1D") return enum_object_type::texture_1d;
		if (type_name == "Texture1DArray") return enum_object_type::texture_1d_array;
		if (type_name == "Texture2D") return enum_object_type::texture_2d;
		if (type_name == "Texture2DArray") return enum_object_type::texture_2d_array;
		if (type_name == "Texture3D") return enum_object_type::texture_3d;
		if (type_name == "Texture2DMS") return enum_object_type::texture_2d_ms;
		if (type_name == "Texture2DMSArray") return enum_object_type::texture_2d_ms_array;
		if (type_name == "TextureCube") return enum_object_type::texture_cube;
		if (type_name == "TextureCubeArray") return enum_object_type::texture_cube_array;
		if (type_name == "RWTexture1D") return enum_object_type::rw_texture_1d;
		if (type_name == "RWTexture1DArray") return enum_object_type::rw_texture_1d_array;
		if (type_name == "RWTexture2D") return enum_object_type::rw_texture_2d;
		if (type_name == "RWTexture2DArray") return enum_object_type::rw_texture_2d_array;
		if (type_name == "RWTexture3D") return enum_object_type::rw_texture_3d;
		
		if (type_name == "InputPatch") return enum_object_type::input_patch;
		if (type_name == "OutputPatch") return enum_object_type::output_patch;
		if (type_name == "ConstantBuffer") return enum_object_type::constant_buffer;
		
		return enum_object_type::none;
	}

	ast_node_type_format* shader_lang_state::create_basic_type_node(const token_data& type_info)
	{
		const auto node = new ast_node_type_format;
		String sub;
		switch (type_info.token_id)
		{
		case TYPE_BOOL:
			node->basic_type = enum_basic_type::tp_bool;
			break;
		case TYPE_UINT:
			node->basic_type = enum_basic_type::tp_uint;
			break;
		case TYPE_INT:
			node->basic_type = enum_basic_type::tp_int;
			break;
		case TYPE_HALF:
			node->basic_type = enum_basic_type::tp_half;
			break;
		case TYPE_FLOAT:
			node->basic_type = enum_basic_type::tp_float;
			break;
		case TYPE_DOUBLE:
			node->basic_type = enum_basic_type::tp_double;
		case TYPE_VOID:
			node->basic_type = enum_basic_type::tp_void;
			break;
		case TYPE_TEXTURE:
			node->basic_type = enum_basic_type::tp_texture;
			break;
		case TYPE_SAMPLER:
			node->basic_type = enum_basic_type::tp_sampler;
			break;
		case TYPE_ID:
			{
				node->basic_type = enum_basic_type::tp_indict; //reference type
				auto symbol = get_global_symbol(type_info.text);
				if (symbol && symbol->node_type == enum_ast_node_type::structure)
				{
					node->indirect_index = static_cast<ast_node_struct*>(symbol)->get_type()->indirect_index;
				}
				break;
			}
		case TYPE_VECTOR:
		case TYPE_MATRIX:
			{
				switch (type_info.text[0])
				{
				case 'b':
					node->basic_type = enum_basic_type::tp_bool;
					sub = type_info.text.substr(4);
					break;
				case 'i':
					node->basic_type = enum_basic_type::tp_int;
					sub = type_info.text.substr(3);
					break;
				case 'u':
					node->basic_type = enum_basic_type::tp_uint;
					sub = type_info.text.substr(4);
					break;
				case 'h':
					node->basic_type = enum_basic_type::tp_half;
					sub = type_info.text.substr(4);
					break;
				case 'f':
					node->basic_type = enum_basic_type::tp_float;
					sub = type_info.text.substr(5);
					break;
				case 'd':
					node->basic_type = enum_basic_type::tp_double;
					sub = type_info.text.substr(6);
					break;
				default: break;
				}
				node->row = sub[0] - '0';
				if (type_info.token_id == TYPE_VECTOR)
				{
					node->type_class = enum_type_class::vector;
				}
				else
				{
					node->type_class = enum_type_class::matrix;
					node->column = sub[2] - '0';
				}
				break;
			}
		default:
			break;
		}

		return node;
	}

	ast_node_type_format* shader_lang_state::create_object_type_node(const token_data& object)
	{
		const auto node = new ast_node_type_format();
		node->basic_type = enum_basic_type::tp_none;
		node->object_type = parse_object_type_from_name(object.text);
		TAssert(node->object_type != enum_object_type::none);
		return node;
	}

	// 创建带模板参数的对象类型节点
	ast_node_type_format* shader_lang_state::create_object_type_node(const token_data& object, const token_data& content)
	{
		ast_node_type_format* node = new ast_node_type_format();
		ast_node_type_format* content_node = create_basic_type_node(content);
		node->packed_flags = content_node->packed_flags;
		LOG("packed_flags: %d", node->packed_flags);
		node->object_type = parse_object_type_from_name(object.text);
		LOG("packed_flags after parse_object_type: %d", node->packed_flags);
		TAssert(node->object_type != enum_object_type::none);
		return node;
	}

	void shader_lang_state::combine_modifier(ast_node_type_format* type, ast_node_type_format* modifier)
	{
		type->qualifiers |= modifier->qualifiers;
	}

	void shader_lang_state::apply_modifier(ast_node_type_format* type, const token_data& modifier)
	{
		// 根据token类型设置相应的限定符标志
		if (modifier.token_id == TOKEN_IN)
		{
			type->is_in = true;
		}
		else if (modifier.token_id == TOKEN_OUT)
		{
			type->is_out = true;
		}
		else if (modifier.token_id == TOKEN_INOUT)
		{
			type->is_inout = true;
		}
		else if (modifier.text == "const")
		{
			type->is_const = true;
		}
		else if (modifier.text == "static")
		{
			type->is_static = true;
		}
		else if (modifier.token_id == TOKEN_SV)
		{
			type->is_semantic = true;
		}
		else
		{
			debug_log("Unknown type qualifier: " + modifier.text);
		}
	}

	ast_node_statement* shader_lang_state::create_declaration_statement(ast_node_type_format* type, const token_data& name,
		const dimensions& dim, ast_node_expression* expr)
	{
		TAssert(get_local_symbol(name.text) == nullptr);
		const auto variable = new ast_node_variable(type, name.text);
		variable->dimension = dim;
		current_scope()->push_symbol(name.text, enum_symbol_type::variable, variable);
		const auto node = new variable_declaration_statement(variable, expr);
		return node;
	}

	ast_node_statement* shader_lang_state::create_return_statement(ast_node_expression* expr)
	{
		return new return_statement(expr);
	}

	ast_node_statement* shader_lang_state::create_break_statement()
	{
		return new break_statement();
	}

	ast_node_statement* shader_lang_state::create_continue_statement()
	{
		return new continue_statement();
	}

	ast_node_statement* shader_lang_state::create_discard_statement()
	{
		return new discard_statement();
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

	ast_node_expression* shader_lang_state::create_constructor_expression(ast_node_type_format* type)
	{
		return new constructor_expression(type);
	}

	void shader_lang_state::append_argument(ast_node_expression* func_call_expr, ast_node_expression* arg_expr)
	{
		TAssert(func_call_expr != nullptr);
		if (func_call_expr->expr_type == enum_expr_type::function_call)
		{
			const auto expr = static_cast<function_call_expression*>(func_call_expr);
			expr->add_argument(arg_expr);
		}
		else if (func_call_expr->expr_type == enum_expr_type::constructor_call)
		{
			const auto expr = static_cast<constructor_expression*>(func_call_expr);
			expr->add_argument(arg_expr);
		}
		else
		{
			TAssert(false && "Expected function call or constructor expression");
		}
	}

	ast_node_expression* shader_lang_state::create_assignment_expression(ast_node_expression* lhs, ast_node_expression* rhs)
	{
		return new assignment_expression(lhs, rhs);
	}

	ast_node_expression* shader_lang_state::create_conditional_expression(ast_node_expression* cond, ast_node_expression* true_expr, ast_node_expression* false_expr)
	{
		return new conditional_expression(cond, true_expr, false_expr);
	}

	ast_node_expression* shader_lang_state::create_chain_expression(ast_node_expression* prev, ast_node_expression* next)
	{
		// 检查prev是否已经是链式表达式
		chain_expression* chain_expr = dynamic_cast<chain_expression*>(prev);
		if (chain_expr)
		{
			// 如果已经是链式表达式，直接添加新表达式
			chain_expr->add_expression(next);
			return chain_expr;
		}
		else
		{
			// 创建新的链式表达式
			chain_expr = new chain_expression(prev);
			chain_expr->add_expression(next);
			return chain_expr;
		}
	}

	ast_node_expression* shader_lang_state::create_cast_expression(ast_node_type_format* target_type, ast_node_expression* operand)
	{
		return new cast_expression(target_type, operand);
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

	ast_node_expression* shader_lang_state::create_index_expression(ast_node_expression* expr,
		ast_node_expression* index_expr)
	{
		return new index_expression(expr, index_expr);
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
		//ast_root->print_ast(0);
		printf("\n");
		String outHlsl;
		ast_root->generate_hlsl(outHlsl);
		printf("-------generate hlsl-------\n%s", outHlsl.c_str());

		/*printf("\n-------DXCompiler-------\n");
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
		}*/
	}



}