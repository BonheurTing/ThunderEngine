#pragma once

#include "Container.h"
#include "Platform.h"
#include "AstNode.h"

namespace Thunder
{
	struct debug_info
	{
		String message;
		int line_no;
		debug_info* next;
	};

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

	enum class enum_shader_stage : uint8
	{
		unknown = 0,
		vertex,
		hull,
		domain,
		pixel,
		compute,
		geometry,
		mesh,
		max
	};

	struct uniform_buffer_definition
	{
		uint16 index = 0xFFFF;
	};
	SHADERLANG_API extern TMap<String, uniform_buffer_definition> GUniformBufferDefinitions;

	SHADERLANG_API extern const TArray<NameHandle> GStaticSamplerNames;

	struct shader_lang_state
	{
		shader_lang_state();
		~shader_lang_state();
		void reset();

		// debug info
		String current_text; // current token text
		parse_location* current_location = nullptr;
		
		// flex bison
		void *scanner = nullptr;	// Flex lexical analyzer context
		//void *parser = nullptr;		// Bison grammar analyzer status

		// Abstract Syntax Tree
		String shader_name;
		ast_node* ast_root = nullptr;
		
		TArray<scope*> symbol_scopes;

		TMap<String, shader_lang_symbol> variants_map; // Variants look-up.

		TArray<ast_node*> custom_types; // for indirect types

		// function stacks.
		//std::vector<ast_node_function*> functions;
		// block stacks.
		TArray<ast_node_block*> block_stacks;

		// runtime context
		ast_node_pass* current_pass = nullptr;
		ast_node_struct* current_structure = nullptr;
		ast_node_function* current_function = nullptr;
		ast_node_archive* current_archive = nullptr;
		TArray<ast_node_variable*> current_variables;
		shader_attributes current_attributes;

		void parsing_archive_begin(const token_data& name);
		ast_node* parsing_archive_end(ast_node* content);
		void add_variable_to_list(ast_node_type_format* type, const token_data& name, ast_node_expression* default_value);
		void add_variant(ast_node_variable* variant);
		void parsing_variable_end(const token_data& name, const token_data& text);

		void parsing_pass_begin(const token_data& name);
		ast_node* parsing_pass_end();
		void add_definition_member(ast_node* def);
		void add_attribute_entry(const token_data& key, const token_data& value);
		void add_stage_entry(const token_data& stage_token, const token_data& entry_name);
		
		void parsing_struct_begin(const token_data& name);
		ast_node_struct* parsing_struct_end();
		void add_struct_member(ast_node_type_format* type, const token_data& name, const token_data& modifier, const parse_location* loc);
		
		void parsing_function_begin(ast_node_type_format* type, const token_data& name);
		ast_node_function* parsing_function_end(const token_data& info, ast_node_block* body);
		void add_function_param(ast_node_type_format* type, const token_data& name, const token_data& modifier, const parse_location* loc);

		void parsing_block_begin();
		ast_node_block* parsing_block_end();
		void add_block_statement(ast_node_statement* statement, const parse_location* loc) const;

		int evaluate_integer_expression(ast_node_expression* expr, const parse_location& loc);

		// type system
		ast_node_type_format* create_basic_type_node(const token_data& type_info);
		ast_node_type_format* create_object_type_node(const token_data& object);
		ast_node_type_format* create_object_type_node(const token_data& object, const token_data& content);
		void combine_modifier(ast_node_type_format* type, ast_node_type_format* modifier);
		void apply_modifier(ast_node_type_format* type, const token_data& modifier);

		// Statement
		ast_node_statement* create_declaration_statement(ast_node_type_format* type, const token_data& name, const dimensions& dim, ast_node_expression* expr);
		ast_node_statement* create_return_statement(ast_node_expression* expr);
		ast_node_statement* create_break_statement();
		ast_node_statement* create_continue_statement();
		ast_node_statement* create_discard_statement();
		ast_node_statement* create_expression_statement(ast_node_expression* expr);
		ast_node_statement* create_condition_statement(ast_node_expression* cond, ast_node_statement* true_stmt, ast_node_statement* false_stmt);
		ast_node_statement* create_for_statement(ast_node_statement* init, ast_node_expression* cond, ast_node_expression* update, ast_node_statement* body);

		// Expression
		ast_node_expression* create_reference_expression(const token_data& name);
		ast_node_expression* create_binary_expression(enum_binary_op op, ast_node_expression* left, ast_node_expression* right);
		ast_node_expression* create_unary_expression(int op, ast_node_expression* operand);
		ast_node_expression* create_compound_assignment_expression(int op, ast_node_expression* lhs, ast_node_expression* rhs);
		ast_node_expression* create_shuffle_or_component_expression(ast_node_expression* expr, const token_data& comp);
		ast_node_expression* create_index_expression(ast_node_expression* expr, ast_node_expression* index_expr);
		ast_node_expression* create_function_call_expression(const token_data& func_name);
		ast_node_expression* create_method_call_expression(ast_node_expression* object, ast_node_expression* post_obj);
		ast_node_expression* create_constructor_expression(ast_node_type_format* type);
		void append_argument(ast_node_expression* func_call_expr, ast_node_expression* arg_expr);
		ast_node_expression* create_assignment_expression(ast_node_expression* lhs, ast_node_expression* rhs);
		ast_node_expression* create_conditional_expression(ast_node_expression* cond, ast_node_expression* true_expr, ast_node_expression* false_expr);
		ast_node_expression* create_chain_expression(ast_node_expression* prev, ast_node_expression* next);
		ast_node_expression* create_cast_expression(ast_node_type_format* target_type, ast_node_expression* operand);
		
		// Constant Expression
		ast_node_expression* create_constant_int_expression(int value);
		ast_node_expression* create_constant_float_expression(float value);
		ast_node_expression* create_constant_bool_expression(bool value);

		// scope & symbol
		void push_scope(scope* in_scope);
		scope* pop_scope();
		scope* current_scope() const;
		ast_node* get_local_symbol(const String& name) const;
		ast_node* get_global_symbol(const String& name);
		enum_symbol_type get_symbol_type(const String& name) const;
		bool is_key_identifier(const String& name) const;

		// error handle
		void debug_log(const String& msg, const parse_location* loc = nullptr) const;
		void post_process_ast() const;

	};
	extern shader_lang_state* sl_state;
}
