#pragma optimize("", off)
#include "AstNode.h"
#include "ShaderLang.h"
#include "Assertion.h"
#include "../Generated/parser.tab.h"
#include<string_view>

namespace Thunder
{
    shader_lang_state* sl_state = nullptr;

    static void print_blank(int indent) {
        for (int i = 0; i < indent; i++) printf("  ");
    }

    void ast_node::generate_dxil()
    {
    }

    scope* ast_node_archive::begin_archive()
    {
        global_scope = new scope(nullptr, this);
        return global_scope.Get();
    }

    void ast_node_archive::end_archive(scope* current)
    {
        TAssert(global_scope == current);
        global_scope.SafeRelease();
    }

    scope* ast_node_struct::begin_structure(scope* outer)
    {
        local_scope = new scope(outer, this);
        /*for (ast_node_variable* member : members)
        {
            local_scope->push_symbol(member->name, enum_symbol_type::variable, member);
        }*/
        return local_scope.Get();
    }

    void ast_node_struct::end_structure(scope* current)
    {
        TAssert(local_scope == current);
        local_scope.SafeRelease();
    }

    scope* ast_node_function::begin_function(scope* outer)
    {
        local_scope = new scope(outer, this);
        return local_scope.Get();
    }

    void ast_node_function::end_function(scope* current)
    {
        TAssert(local_scope == current);
        local_scope.SafeRelease();
    }

    scope* ast_node_block::begin_block(scope* outer)
    {
        local_scope = new scope(outer, this);
        return local_scope.Get();
    }

    void ast_node_block::end_block(scope* current)
    {
        TAssert(local_scope == current);
        local_scope.SafeRelease();
    }


#pragma region HLSL


    static const String TemplateTypeNames[static_cast<size_t>(enum_object_type::object_num)] =
    {
        "none",
        // Buffer object
        "Buffer",
        "ByteAddressBuffer",
        "StructuredBuffer",
        "AppendStructuredBuffer",
        "ConsumeStructuredBuffer",
        "RWBuffer",
        "RWByteAddressBuffer",
        "RWStructuredBuffer",
        // Texture object
        "Texture1D",
        "Texture1DArray",
        "Texture2D",
        "Texture2DArray",
        "Texture2DMS",
        "Texture2DMSArray",
        "Texture3D",
        "TextureCube",
        "TextureCubeArray",
        "RWTexture1D",
        "RWTexture1DArray",
        "RWTexture2D",
        "RWTexture2DArray",
        "RWTexture3D",
        // Tessellation in/out
        "InputPatch",
        "OutputPatch",
        "ConstantBuffer",
    };

    String ast_node_type_format::get_type_text_internal(shader_codegen_state& state) const
    {
        String text = "";
        switch (basic_type)
        {
            case enum_basic_type::tp_void: text = "void"; break;
            case enum_basic_type::tp_bool: text = "bool"; break;
            case enum_basic_type::tp_int: text = "int"; break;
            case enum_basic_type::tp_uint: text = "uint"; break;
            case enum_basic_type::tp_half: text = "half"; break;
            case enum_basic_type::tp_float: text = "float"; break;
            case enum_basic_type::tp_double: text = "double"; break;
            case enum_basic_type::tp_none:
                text = TemplateTypeNames[static_cast<int>(object_type)];
                break;
            case enum_basic_type::tp_indict:
            {
                ast_node* node = state.custom_types[indirect_index];
                if (node && node->node_type == enum_ast_node_type::structure)
                {
                    text = static_cast<ast_node_struct*>(node)->get_name();
                }
                else
                {
                    TAssert(false);
                }
                break;
            }
            default: break;
        }
        if (type_class == enum_type_class::vector
            || type_class == enum_type_class::matrix)
        {
            text += std::to_string(row);
        }
        if (type_class == enum_type_class::matrix)
        {
            text += 'x';
            text += std::to_string(column);
        }
        return text;
    }

    String ast_node_type_format::get_type_text(shader_codegen_state& state) const
    {
        String text = "";
        if (object_type != enum_object_type::none)
        {
            text = TemplateTypeNames[static_cast<size_t>(object_type)];
            if (basic_type != enum_basic_type::tp_none)
            {
                text += '<';
                text += get_type_text_internal(state);
                text += '>';
            }
        }
        else
        {
            text = get_type_text_internal(state);
        }
        return text;
    }

    String ast_node_type_format::get_type_format_text() const
    {
        auto state = shader_codegen_state{};
        if (object_type != enum_object_type::none)
        {
            if (basic_type != enum_basic_type::tp_none)
            {
                return get_type_text_internal(state);
            }
        }
        return "";
    }

    void ast_node_type_format::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        if (is_in)
        {
            outResult += "in ";
        }
        if (is_out)
        {
            outResult += "out ";
        }
        if (is_inout)
        {
            outResult += "inout ";
        }
        if (is_static)
        {
            outResult += "static ";
        }
        if (is_const)
        {
            outResult += "const ";
        }

        outResult += get_type_text(state);
    }

    void ast_node_variable::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        type->generate_hlsl(outResult, state);
        outResult += " " + name;
        if (type->is_semantic)
        {
            outResult += " : " + semantic;
        }
    }

    void ast_node_archive::reflect_pass_cb_parameters()
    {
        auto const& uniform_buffer_entry = uniform_buffer_parameters.find("Pass");
        if (uniform_buffer_entry == uniform_buffer_parameters.end()) [[uniform_buffer]] return;
        auto const& buffer_parameters = uniform_buffer_entry->second;
        for (const auto pass : passes)
        {
            pass_cb_parameters[pass->get_name()] = {};
        }
        for (auto const& parameter : buffer_parameters)
        {
            for (const auto pass : passes)
            {
                // if (pass->fine_parameter(parameter->name)) // pass cb optimize: temporarily not opened for debug.
                {
                    pass_cb_parameters.at(pass->get_name()).push_back(parameter);
                }
            }
        }
    }

    void ast_node_archive::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        if (state.sub_shader_name.IsEmpty())
        {
            TAssertf(false, "Empty subshader.");
            return;
        }
        TArray<ast_node_variable*> objects;
        for (auto const& uniform_buffer_entry : uniform_buffer_parameters)
        {
            String const& buffer_name = uniform_buffer_entry.first;
            TArray<ast_node_variable*> buffer_parameters;
            auto uniform_buffer_definition_it = GUniformBufferDefinitions.find(buffer_name);
            if (uniform_buffer_definition_it == GUniformBufferDefinitions.end())
            {
                TAssertf(false, "Unknown uniform buffer : \"%s\".", buffer_name.c_str());
                continue;
            }
            uint16 const buffer_index = uniform_buffer_definition_it->second.index;
            if (buffer_name == "Pass")
            {
                outResult += "cbuffer cb" + state.sub_shader_name.ToString() + " : register(b" + std::to_string(buffer_index) + ", space0)\n{\n";
                buffer_parameters = pass_cb_parameters.at(state.sub_shader_name.ToString());
            }
            else
            {
                outResult += "cbuffer cb" + buffer_name + " : register(b" + std::to_string(buffer_index) + ", space0)\n{\n";
                buffer_parameters = uniform_buffer_entry.second;
            }
            for (auto const& parameter : buffer_parameters)
            {
                if (parameter->type->is_object())
                {
                    objects.push_back(parameter);
                    continue;
                }
                parameter->generate_hlsl(outResult, state);
                outResult += ";\n";
            }

            outResult += "};\n\n";
        }
        int object_index = 0;
        for (const auto& obj : objects)
        {
            obj->type->generate_hlsl(outResult, state);
            outResult += " " + obj->name + " : register(t" + std::to_string(object_index++) + ");\n";
        }

        // custom sampler
        for (size_t i = 0; i < sampler_parameters.size(); i++)
        {
            outResult += "SamplerState " + sampler_parameters[i]->name + " : register(s" + std::to_string(i) + ", space0);\n";
        }
        // static sampler
        for (size_t i = 0; i < GStaticSamplerNames.size(); i++)
        {
            outResult += "SamplerState " + GStaticSamplerNames[i].ToString() + " : register(s" + std::to_string(i) + ", space1000);\n";
        }

        outResult += "\n";

        bool find_valid_pass = false;
        for (const auto pass : passes)
        {
            if (pass->get_name() == state.sub_shader_name.ToString())
            {
                find_valid_pass = true;
                pass->generate_hlsl(outResult, state);
                outResult += "\n";
            }
        }
        TAssertf(find_valid_pass, "Unknown subshader name : \"%s\".", state.sub_shader_name);
    }

    void ast_node_pass::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        for (const auto& def : structures)
        {
            def->generate_hlsl(outResult, state);
            outResult += "\n";
        }
        for (const auto& def : functions)
        {
            def->generate_hlsl(outResult, state);
            outResult += "\n";
        }
    }
    
    void ast_node_struct::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        outResult += "struct " + name + " {\n";
        for (const auto& member : members)
        {
            member->generate_hlsl(outResult, state);
            outResult += ";\n";
        }
        outResult += "};\n";
    }

    void ast_node_function::generate_hlsl(std::string& outResult, shader_codegen_state& state)
    {
        String paramList;
        for (const auto& param : params)
        {
            if (!paramList.empty())
            {
                paramList += ", ";
            }
            param->generate_hlsl(paramList, state);
        }

        String funcSignature;
        return_type->generate_hlsl(funcSignature, state);
        funcSignature += " " + func_name.ToString() + "(" + paramList + ")";

        if (!semantic.empty())
        {
            funcSignature += " : " + semantic + "\n";
        }
        else
        {
            funcSignature += "\n";
        }

        String funcBody;
        body->generate_hlsl(funcBody, state);
        outResult += funcSignature + "{\n" + funcBody + "}\n";
    }

    void ast_node_block::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        for(const auto stat : statements)
        {
            stat->generate_hlsl(outResult, state);
        }
    }

    void variable_declaration_statement::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        variable->generate_hlsl(outResult, state);
        
        if (decl_expr)
        {
            outResult += " = ";
            decl_expr->generate_hlsl(outResult, state);
        }
        
        outResult += ";\n";
    }

    void return_statement::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        outResult += "return ";
        if (ret_value != nullptr)
        {
            ret_value->generate_hlsl(outResult, state);
        }
        outResult += ";\n";
    }

    void break_statement::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        outResult += "break;\n";
    }

    void continue_statement::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        outResult += "continue;\n";
    }

    void discard_statement::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        outResult += "discard;\n";
    }

    void expression_statement::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        if (expr != nullptr)
        {
            expr->generate_hlsl(outResult, state);
            outResult += ";\n";
        }
    }

    void condition_statement::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        if (!condition)
        {
            return;
        }

        const auto cond_ret = condition->evaluate(state);
        if (cond_ret.result_type == enum_eval_result_type::constant_bool)
        {
            if (cond_ret.bool_value)
            {
                if (true_branch)
                {
                    true_branch->generate_hlsl(outResult, state);
                }
            }
            else
            {
                if (false_branch)
                {
                    false_branch->generate_hlsl(outResult, state);
                }
            }
        }
        else if (cond_ret.result_type == enum_eval_result_type::constant_int)
        {
            if (cond_ret.int_value > 0)
            {
                if (true_branch)
                {
                    true_branch->generate_hlsl(outResult, state);
                }
            }
            else
            {
                if (false_branch)
                {
                    false_branch->generate_hlsl(outResult, state);
                }
            }
        }
        else if (cond_ret.result_type == enum_eval_result_type::constant_float)
        {
            if (cond_ret.float_value > 0.0f)
            {
                if (true_branch)
                {
                    true_branch->generate_hlsl(outResult, state);
                }
            }
            else
            {
                if (false_branch)
                {
                    false_branch->generate_hlsl(outResult, state);
                }
            }
        }
        else
        {
            if (true_branch)
            {
                outResult += "if (";
                condition->generate_hlsl(outResult, state);
                outResult += ") {\n";
                true_branch->generate_hlsl(outResult, state);
                outResult += "}\n";
            }
            if (false_branch)
            {
                outResult += "else {\n";
                false_branch->generate_hlsl(outResult, state);
                outResult += "}\n";
            }
        }
    }

    void binary_expression::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        TAssertf(left != nullptr && right != nullptr, "Binary operation left or right is null");
        outResult += "(";
        left->generate_hlsl(outResult, state);
        switch (op) {
        case enum_binary_op::add: outResult += " + "; break;
        case enum_binary_op::sub: outResult += " - "; break;
        case enum_binary_op::mul: outResult += " * "; break;
        case enum_binary_op::div: outResult += " / "; break;
        case enum_binary_op::mod: outResult += " % "; break;
        case enum_binary_op::bit_and: outResult += " & "; break;
        case enum_binary_op::bit_or: outResult += " | "; break;
        case enum_binary_op::bit_xor: outResult += " ^ "; break;
        case enum_binary_op::left_shift: outResult += " << "; break;
        case enum_binary_op::right_shift: outResult += " >> "; break;
        case enum_binary_op::logical_and: outResult += " && "; break;
        case enum_binary_op::logical_or: outResult += " || "; break;
        case enum_binary_op::equal: outResult += " == "; break;
        case enum_binary_op::not_equal: outResult += " != "; break;
        case enum_binary_op::less: outResult += " < "; break;
        case enum_binary_op::less_equal: outResult += " <= "; break;
        case enum_binary_op::greater: outResult += " > "; break;
        case enum_binary_op::greater_equal: outResult += " >= "; break;
        case enum_binary_op::undefined: break;
        }
        right->generate_hlsl(outResult, state);
        outResult += ")";
    }

    void shuffle_expression::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        prefix->generate_hlsl(outResult, state);
        outResult += ".";
        for (const char i : order)
        {
            if (i != 0)
            {
                outResult += i;
            }
            else
            {
                break;
            }
        }
    }

    void component_expression::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        component_name->generate_hlsl(outResult, state);
        outResult += ".";
        outResult += identifier;
    }

    void index_expression::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        target->generate_hlsl(outResult, state);
        outResult += "[";
        if (index)
        {
            index->generate_hlsl(outResult, state);
        }
        outResult += "]";
    }

    void reference_expression::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        outResult += identifier;
    }

    void constant_int_expression::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        outResult += std::to_string(value);
    }

    void constant_float_expression::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        outResult += std::to_string(value) + "f";
    }

    void constant_bool_expression::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        outResult += value ? "true" : "false";
    }

    void for_statement::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        outResult += "for (";
        if (init_stmt)
        {
            String init_str;
            init_stmt->generate_hlsl(init_str, state);
            // Remove trailing semicolon and newline from init statement
            if (!init_str.empty() && init_str.back() == '\n')
                init_str.pop_back();
            if (!init_str.empty() && init_str.back() == ';')
                init_str.pop_back();
            outResult += init_str;
        }
        outResult += "; ";
        
        if (condition)
        {
            condition->generate_hlsl(outResult, state);
        }
        outResult += "; ";
        
        if (update_expr)
        {
            update_expr->generate_hlsl(outResult, state);
        }
        outResult += ") {\n";
        
        if (body_stmt)
        {
            body_stmt->generate_hlsl(outResult, state);
        }
        outResult += "}\n";
    }

    void function_call_expression::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        outResult += function_name + "(";
        String arguments_string;
        for (const auto argument : arguments)
        {
            if (arguments_string.length() > 0)
            {
                arguments_string += ", ";
            }
            argument->generate_hlsl(arguments_string, state);
        }
        outResult += arguments_string;
        outResult += ")";
    }

    void method_call_expression::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        object->generate_hlsl(outResult, state);
        outResult += "." ;
        function_call_exp->generate_hlsl(outResult, state);
    }

    void constructor_expression::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        if (constructor_type)
        {
            constructor_type->generate_hlsl(outResult, state);
        }
        outResult += "(";
        String arguments_string;
        for (const auto argument : arguments)
        {
            if (arguments_string.length() > 0)
            {
                arguments_string += ", ";
            }
            argument->generate_hlsl(arguments_string, state);
        }
        outResult += arguments_string;
        outResult += ")";
    }

    void assignment_expression::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        if (left_expr)
        {
            left_expr->generate_hlsl(outResult, state);
        }
        outResult += " = ";
        if (right_expr)
        {
            right_expr->generate_hlsl(outResult, state);
        }
    }

    void conditional_expression::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        if (condition)
        {
            condition->generate_hlsl(outResult, state);
        }
        outResult += " ? ";
        if (true_expression)
        {
            true_expression->generate_hlsl(outResult, state);
        }
        outResult += " : ";
        if (false_expression)
        {
            false_expression->generate_hlsl(outResult, state);
        }
    }

    void unary_expression::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        switch (op) {
        case enum_unary_op::pre_inc:
            outResult += "++";
            if (operand) operand->generate_hlsl(outResult, state);
            break;
        case enum_unary_op::pre_dec:
            outResult += "--";
            if (operand) operand->generate_hlsl(outResult, state);
            break;
        case enum_unary_op::post_inc:
            if (operand) operand->generate_hlsl(outResult, state);
            outResult += "++";
            break;
        case enum_unary_op::post_dec:
            if (operand) operand->generate_hlsl(outResult, state);
            outResult += "--";
            break;
        case enum_unary_op::positive:
            outResult += "+";
            if (operand) operand->generate_hlsl(outResult, state);
            break;
        case enum_unary_op::negative:
            outResult += "-";
            if (operand) operand->generate_hlsl(outResult, state);
            break;
        case enum_unary_op::logical_not:
            outResult += "!";
            if (operand) operand->generate_hlsl(outResult, state);
            break;
        case enum_unary_op::bit_not:
            outResult += "~";
            if (operand) operand->generate_hlsl(outResult, state);
            break;
        case enum_unary_op::undefined:
            break;
        }
    }

    void compound_assignment_expression::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        if (left_expr)
        {
            left_expr->generate_hlsl(outResult, state);
        }
        switch (op) {
        case enum_assignment_op::assign: outResult += " = "; break;
        case enum_assignment_op::add_assign: outResult += " += "; break;
        case enum_assignment_op::sub_assign: outResult += " -= "; break;
        case enum_assignment_op::mul_assign: outResult += " *= "; break;
        case enum_assignment_op::div_assign: outResult += " /= "; break;
        case enum_assignment_op::mod_assign: outResult += " %= "; break;
        case enum_assignment_op::lshift_assign: outResult += " <<= "; break;
        case enum_assignment_op::rshift_assign: outResult += " >>= "; break;
        case enum_assignment_op::and_assign: outResult += " &= "; break;
        case enum_assignment_op::or_assign: outResult += " |= "; break;
        case enum_assignment_op::xor_assign: outResult += " ^= "; break;
        case enum_assignment_op::undefined: break;
        }
        if (right_expr)
        {
            right_expr->generate_hlsl(outResult, state);
        }
    }

    void chain_expression::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        outResult += "{";
        for (size_t i = 0; i < expressions.size(); ++i)
        {
            if (i > 0)
            {
                outResult += ", ";
            }
            if (expressions[i])
            {
                expressions[i]->generate_hlsl(outResult, state);
            }
        }
        outResult += "}";
    }

    void cast_expression::generate_hlsl(String& outResult, shader_codegen_state& state)
    {
        outResult += "(";
        if (cast_type)
        {
            cast_type->generate_hlsl(outResult, state);
        }
        outResult += ")";
        if (operand)
        {
            operand->generate_hlsl(outResult, state);
        }
    }

#pragma endregion // HLSL

#pragma region TO_STRING

    String ast_node_expression::to_string() const
    {
        return "";
    }

    String binary_expression::to_string() const
    {
        String result = "(";
        if (left)
        {
            result += left->to_string();
        }
        switch (op)
        {
        case enum_binary_op::add: result += " + "; break;
        case enum_binary_op::sub: result += " - "; break;
        case enum_binary_op::mul: result += " * "; break;
        case enum_binary_op::div: result += " / "; break;
        case enum_binary_op::mod: result += " % "; break;
        case enum_binary_op::bit_and: result += " & "; break;
        case enum_binary_op::bit_or: result += " | "; break;
        case enum_binary_op::bit_xor: result += " ^ "; break;
        case enum_binary_op::left_shift: result += " << "; break;
        case enum_binary_op::right_shift: result += " >> "; break;
        case enum_binary_op::logical_and: result += " && "; break;
        case enum_binary_op::logical_or: result += " || "; break;
        case enum_binary_op::equal: result += " == "; break;
        case enum_binary_op::not_equal: result += " != "; break;
        case enum_binary_op::less: result += " < "; break;
        case enum_binary_op::less_equal: result += " <= "; break;
        case enum_binary_op::greater: result += " > "; break;
        case enum_binary_op::greater_equal: result += " >= "; break;
        case enum_binary_op::undefined: break;
        }
        if (right)
        {
            result += right->to_string();
        }
        result += ")";
        return result;
    }

    String shuffle_expression::to_string() const
    {
        String result;
        if (prefix)
        {
            result += prefix->to_string();
        }
        result += ".";
        for (const char i : order)
        {
            if (i != 0)
            {
                result += i;
            }
            else
            {
                break;
            }
        }
        return result;
    }

    String component_expression::to_string() const
    {
        String result;
        if (component_name)
        {
            result += component_name->to_string();
        }
        result += ".";
        result += identifier;
        return result;
    }

    String index_expression::to_string() const
    {
        String result;
        if (target)
        {
            result += target->to_string();
        }
        result += "[";
        if (index)
        {
            result += index->to_string();
        }
        result += "]";
        return result;
    }

    String reference_expression::to_string() const
    {
        return identifier;
    }

    String constant_int_expression::to_string() const
    {
        return std::to_string(value);
    }

    String constant_float_expression::to_string() const
    {
        return std::to_string(value) + "f";
    }

    String constant_bool_expression::to_string() const
    {
        return value ? "true" : "false";
    }

    String function_call_expression::to_string() const
    {
        String result = function_name + "(";
        for (size_t i = 0; i < arguments.size(); ++i)
        {
            if (i > 0)
            {
                result += ", ";
            }
            if (arguments[i])
            {
                result += arguments[i]->to_string();
            }
        }
        result += ")";
        return result;
    }

    String method_call_expression::to_string() const
    {
        String result;
        if (object)
        {
            result += object->to_string();
        }
        result += ".";
        if (function_call_exp)
        {
            result += function_call_exp->to_string();
        }
        return result;
    }

    String constructor_expression::to_string() const
    {
        auto state = shader_codegen_state{};
        String result;
        if (constructor_type)
        {
            result += constructor_type->get_type_text(state);
        }
        result += "(";
        for (size_t i = 0; i < arguments.size(); ++i)
        {
            if (i > 0)
            {
                result += ", ";
            }
            if (arguments[i])
            {
                result += arguments[i]->to_string();
            }
        }
        result += ")";
        return result;
    }

    String assignment_expression::to_string() const
    {
        String result;
        if (left_expr)
        {
            result += left_expr->to_string();
        }
        result += " = ";
        if (right_expr)
        {
            result += right_expr->to_string();
        }
        return result;
    }

    String conditional_expression::to_string() const
    {
        String result;
        if (condition)
        {
            result += condition->to_string();
        }
        result += " ? ";
        if (true_expression)
        {
            result += true_expression->to_string();
        }
        result += " : ";
        if (false_expression)
        {
            result += false_expression->to_string();
        }
        return result;
    }

    String unary_expression::to_string() const
    {
        String result;
        switch (op)
        {
        case enum_unary_op::pre_inc:
            result += "++";
            if (operand) result += operand->to_string();
            break;
        case enum_unary_op::pre_dec:
            result += "--";
            if (operand) result += operand->to_string();
            break;
        case enum_unary_op::post_inc:
            if (operand) result += operand->to_string();
            result += "++";
            break;
        case enum_unary_op::post_dec:
            if (operand) result += operand->to_string();
            result += "--";
            break;
        case enum_unary_op::positive:
            result += "+";
            if (operand) result += operand->to_string();
            break;
        case enum_unary_op::negative:
            result += "-";
            if (operand) result += operand->to_string();
            break;
        case enum_unary_op::logical_not:
            result += "!";
            if (operand) result += operand->to_string();
            break;
        case enum_unary_op::bit_not:
            result += "~";
            if (operand) result += operand->to_string();
            break;
        case enum_unary_op::undefined:
            break;
        }
        return result;
    }

    String compound_assignment_expression::to_string() const
    {
        String result;
        if (left_expr)
        {
            result += left_expr->to_string();
        }
        switch (op)
        {
        case enum_assignment_op::assign: result += " = "; break;
        case enum_assignment_op::add_assign: result += " += "; break;
        case enum_assignment_op::sub_assign: result += " -= "; break;
        case enum_assignment_op::mul_assign: result += " *= "; break;
        case enum_assignment_op::div_assign: result += " /= "; break;
        case enum_assignment_op::mod_assign: result += " %= "; break;
        case enum_assignment_op::lshift_assign: result += " <<= "; break;
        case enum_assignment_op::rshift_assign: result += " >>= "; break;
        case enum_assignment_op::and_assign: result += " &= "; break;
        case enum_assignment_op::or_assign: result += " |= "; break;
        case enum_assignment_op::xor_assign: result += " ^= "; break;
        case enum_assignment_op::undefined: break;
        }
        if (right_expr)
        {
            result += right_expr->to_string();
        }
        return result;
    }

    String chain_expression::to_string() const
    {
        String result = "{";
        for (size_t i = 0; i < expressions.size(); ++i)
        {
            if (i > 0)
            {
                result += ", ";
            }
            if (expressions[i])
            {
                result += expressions[i]->to_string();
            }
        }
        result += "}";
        return result;
    }

    String cast_expression::to_string() const
    {
        auto state = shader_codegen_state{};
        String result = "(";
        if (cast_type)
        {
            result += cast_type->get_type_text(state);
        }
        result += ")";
        if (operand)
        {
            result += operand->to_string();
        }
        return result;
    }

#pragma endregion // TO_STRING

#pragma region PRINT_AST
    void ast_node_type_format::print_ast(int indent)
    {
        print_blank(indent);
        String type_text;
        shader_codegen_state temp_state{};
        generate_hlsl(type_text, temp_state);
        printf("Type: %s", type_text.c_str());
    }

    void ast_node_variable::print_ast(int indent)
    {
        type->print_ast(indent);
        printf(" Name: %s", name.c_str());
        if (type->is_semantic)
        {
            printf(" Semantic: %s", semantic.c_str());
        }
        printf("\n");
    }

    void ast_node_archive::print_ast(int indent)
    {
        for (const auto pass : passes)
        {
            pass->print_ast(indent);
        }
    }

    void ast_node_pass::print_ast(int indent)
    {
        print_blank(indent);
        if (!name.empty())
        {
            printf("Pass: \"%s\"\n", name.c_str());
        }
        else
        {
            printf("Pass:\n");
        }

        if (!attributes.mesh_draw_type.empty() ||
            attributes.depth_test != enum_depth_test::undefined ||
            attributes.blend_mode != enum_blend_mode::undefined)
        {
            print_blank(indent + 1);
            printf("Attributes:\n");

            if (!attributes.mesh_draw_type.empty())
            {
                print_blank(indent + 2);
                printf("MeshDrawType: \"%s\"\n", attributes.mesh_draw_type.c_str());
            }

            if (attributes.depth_test != enum_depth_test::undefined)
            {
                print_blank(indent + 2);
                printf("DepthTest: ");
                switch (attributes.depth_test)
                {
                case enum_depth_test::never: printf("Never\n"); break;
                case enum_depth_test::less: printf("Less\n"); break;
                case enum_depth_test::equal: printf("Equal\n"); break;
                case enum_depth_test::less_equal: printf("LessEqual\n"); break;
                case enum_depth_test::greater: printf("Greater\n"); break;
                case enum_depth_test::not_equal: printf("NotEqual\n"); break;
                case enum_depth_test::greater_equal: printf("GreaterEqual\n"); break;
                case enum_depth_test::always: printf("Always\n"); break;
                default: printf("Undefined\n"); break;
                }
            }

            if (attributes.blend_mode != enum_blend_mode::undefined)
            {
                print_blank(indent + 2);
                printf("BlendMode: ");
                switch (attributes.blend_mode)
                {
                case enum_blend_mode::opaque: printf("Opaque\n"); break;
                case enum_blend_mode::translucent: printf("Translucent\n"); break;
                default: printf("Undefined\n"); break;
                }
            }
        }

        for (const auto def : structures)
        {
            def->print_ast(indent + 1);
        }
        for (const auto def : functions)
        {
            def->print_ast(indent + 1);
        }
    }

    void ast_node_struct::print_ast(int indent)
    {
        print_blank(indent);
        printf("Struct: %s\n", name.c_str());
        print_blank(indent + 1);
        printf("Members:\n");
        for (const auto& member : members)
        {
            member->print_ast(indent + 2);
        }
    }

    void ast_node_function::print_ast(int indent)
    {
        print_blank(indent);
        printf("Function:\n");
        print_blank(indent);
        printf("Signature:\n");
        print_blank(indent + 1);
        printf("FuncName: %s\n", func_name.c_str());
        print_blank(indent + 1);
        printf("Return");
        return_type->print_ast(0);
        printf("\n");
        print_blank(indent + 1);
        printf("ParamList(Count %d):\n", static_cast<int>(params.size()));
        for(const auto& param : params)
        {
            param->print_ast(indent + 2);
        }
        body->print_ast(indent + 1);
    }

    void ast_node_block::print_ast(int indent)
    {
        for (const auto stat : statements)
        {
            stat->print_ast(indent);
        }
    }

    void variable_declaration_statement::print_ast(int indent)
    {
        print_blank(indent);
        printf("VarDecl: {\n");
        variable->print_ast(indent + 1);

        if (decl_expr)
        {
            printf("DeclExpr:\n");
            decl_expr->print_ast(indent + 1);
        }
        
        print_blank(indent);
        printf("}\n");
    }

    void return_statement::print_ast(int indent)
    {
        print_blank(indent);
        printf("return: { \n");
        if (ret_value != nullptr)
        {
            ret_value->print_ast(indent + 1);
        }
        else
        {
            printf(";\n");
        }
        print_blank(indent);
        printf("}\n");
    }

    void break_statement::print_ast(int indent)
    {
        print_blank(indent);
        printf("break\n");
    }

    void continue_statement::print_ast(int indent)
    {
        print_blank(indent);
        printf("continue\n");
    }

    void discard_statement::print_ast(int indent)
    {
        print_blank(indent);
        printf("discard\n");
    }

    void expression_statement::print_ast(int indent)
    {
        print_blank(indent);
        printf("Expression: {\n");
        if (expr != nullptr)
        {
            expr->print_ast(indent + 1);
        }
        else
        {
            printf(";\n");
        }
        print_blank(indent);
        printf("}\n");
    }

    void condition_statement::print_ast(int indent)
    {
        if (!condition)
            return;

        if (true_branch)
        {
            print_blank(indent);
            printf("if: (\n");
            condition->print_ast(indent + 1);
            print_blank(indent);
            printf(") {\n");
            true_branch->print_ast(indent + 1);
            print_blank(indent);
            printf("}\n");
        }
        if (false_branch)
        {
            print_blank(indent);
            printf("else {\n");
            false_branch->print_ast(indent + 1);
            print_blank(indent);
            printf("}\n");
        }
    }

    void binary_expression::print_ast(int indent)
    {
        TAssertf(left != nullptr && right != nullptr, "Binary operation left or right is null");
        print_blank(indent);
        printf("BinaryOp: ");
        switch (op) {
        case enum_binary_op::add: printf("+\n"); break;
        case enum_binary_op::sub: printf("-\n"); break;
        case enum_binary_op::mul: printf("*\n"); break;
        case enum_binary_op::div: printf("/\n"); break;
        case enum_binary_op::undefined: break;
        }
        left->print_ast(indent + 1);
        right->print_ast(indent + 1);
    }

    void shuffle_expression::print_ast(int indent)
    {
        print_blank(indent);
        printf("Shuffle: {\n");
        prefix->print_ast(indent + 1);
        print_blank(indent + 1);
        printf(".");
        for (const char i : order)
        {
            if (i != 0)
            {
                printf("%c", i);
            }
            else
            {
                break;
            }
        }
        print_blank(indent);
        printf("}\n");
    }

    void component_expression::print_ast(int indent)
    {
        print_blank(indent);
        printf("Component: {\n");
        component_name->print_ast(indent + 1);
        print_blank(indent + 1);
        printf(".%s\n", identifier.c_str());
        print_blank(indent);
        printf("}\n");
    }

    void index_expression::print_ast(int indent)
    {
        print_blank(indent);
        printf("Index: {\n");
        if (target)
        {
            target->print_ast(indent + 1);
        }
        print_blank(indent + 1);
        printf("[\n");
        if (index)
        {
            index->print_ast(indent + 2);
        }
        print_blank(indent + 1);
        printf("]\n");
        print_blank(indent);
        printf("}\n");
    }

    void reference_expression::print_ast(int indent)
    {
        print_blank(indent);
        printf("%s\n", identifier.c_str());
    }

    void constant_int_expression::print_ast(int indent)
    {
        print_blank(indent);
        printf("ConstantInt: %d\n", value);
    }

    void constant_float_expression::print_ast(int indent)
    {
        print_blank(indent);
        printf("ConstantFloat: %f\n", value);
    }

    void constant_bool_expression::print_ast(int indent)
    {
        print_blank(indent);
        printf("ConstantBool: %s\n", value ? "true" : "false");
    }

    void for_statement::print_ast(int indent)
    {
        print_blank(indent);
        printf("ForStatement:\n");
        print_blank(indent + 1);
        printf("Init:\n");
        if (init_stmt)
        {
            init_stmt->print_ast(indent + 2);
        }
        print_blank(indent + 1);
        printf("Condition:\n");
        if (condition)
        {
            condition->print_ast(indent + 2);
        }
        print_blank(indent + 1);
        printf("Update:\n");
        if (update_expr)
        {
            update_expr->print_ast(indent + 2);
        }
        print_blank(indent + 1);
        printf("Body:\n");
        if (body_stmt)
        {
            body_stmt->print_ast(indent + 2);
        }
    }

    void function_call_expression::print_ast(int indent)
    {
        print_blank(indent);
        printf("FunctionCall: %s\n", function_name.c_str());
        printf("Arguments:\n");
        for (const auto argument : arguments)
        {
            argument->print_ast(indent + 1);
        }
    }

    void method_call_expression::print_ast(int indent)
    {
        print_blank(indent);
        printf("MethodCall:\n");
        print_blank(indent + 1);
        printf("Object:\n");
        if (object)
        {
            object->print_ast(indent + 2);
        }
        print_blank(indent + 1);
        printf("Call:\n");
        if (function_call_exp)
        {
            function_call_exp->print_ast(indent + 2);
        }
    }

    void constructor_expression::print_ast(int indent)
    {
        print_blank(indent);
        printf("Constructor: ");
        if (constructor_type)
        {
            constructor_type->print_ast(0);
        }
        printf("\nArguments:\n");
        for (const auto argument : arguments)
        {
            argument->print_ast(indent + 1);
        }
    }

    void assignment_expression::print_ast(int indent)
    {
        print_blank(indent);
        printf("Assignment:\n");
        print_blank(indent + 1);
        printf("Left:\n");
        if (left_expr)
        {
            left_expr->print_ast(indent + 2);
        }
        print_blank(indent + 1);
        printf("Right:\n");
        if (right_expr)
        {
            right_expr->print_ast(indent + 2);
        }
    }

    void conditional_expression::print_ast(int indent)
    {
        print_blank(indent);
        printf("ConditionalExpr:\n");
        print_blank(indent + 1);
        printf("Condition:\n");
        if (condition)
        {
            condition->print_ast(indent + 2);
        }
        print_blank(indent + 1);
        printf("TrueExpr:\n");
        if (true_expression)
        {
            true_expression->print_ast(indent + 2);
        }
        print_blank(indent + 1);
        printf("FalseExpr:\n");
        if (false_expression)
        {
            false_expression->print_ast(indent + 2);
        }
    }

    void unary_expression::print_ast(int indent)
    {
        print_blank(indent);
        printf("UnaryOp: ");
        switch (op) {
        case enum_unary_op::pre_inc: printf("++pre\n"); break;
        case enum_unary_op::pre_dec: printf("--pre\n"); break;
        case enum_unary_op::post_inc: printf("post++\n"); break;
        case enum_unary_op::post_dec: printf("post--\n"); break;
        case enum_unary_op::positive: printf("+\n"); break;
        case enum_unary_op::negative: printf("-\n"); break;
        case enum_unary_op::logical_not: printf("!\n"); break;
        case enum_unary_op::bit_not: printf("~\n"); break;
        default: printf("undefined\n"); break;
        }
        if (operand)
        {
            operand->print_ast(indent + 1);
        }
    }

    void compound_assignment_expression::print_ast(int indent)
    {
        print_blank(indent);
        printf("CompoundAssignment: ");
        switch (op) {
        case enum_assignment_op::add_assign: printf("+=\n"); break;
        case enum_assignment_op::sub_assign: printf("-=\n"); break;
        case enum_assignment_op::mul_assign: printf("*=\n"); break;
        case enum_assignment_op::div_assign: printf("/=\n"); break;
        case enum_assignment_op::mod_assign: printf("%%=\n"); break;
        case enum_assignment_op::lshift_assign: printf("<<=\n"); break;
        case enum_assignment_op::rshift_assign: printf(">>=\n"); break;
        case enum_assignment_op::and_assign: printf("&=\n"); break;
        case enum_assignment_op::or_assign: printf("|=\n"); break;
        case enum_assignment_op::xor_assign: printf("^=\n"); break;
        default: printf("undefined\n"); break;
        }
        print_blank(indent + 1);
        printf("Left:\n");
        if (left_expr)
        {
            left_expr->print_ast(indent + 2);
        }
        print_blank(indent + 1);
        printf("Right:\n");
        if (right_expr)
        {
            right_expr->print_ast(indent + 2);
        }
    }

    void chain_expression::print_ast(int indent)
    {
        print_blank(indent);
        printf("ChainExpression:\n");
        for (size_t i = 0; i < expressions.size(); ++i)
        {
            print_blank(indent + 1);
            printf("Expression[%zu]:\n", i);
            if (expressions[i])
            {
                expressions[i]->print_ast(indent + 2);
            }
        }
    }

    void cast_expression::print_ast(int indent)
    {
        print_blank(indent);
        printf("CastExpression:\n");
        print_blank(indent + 1);
        printf("TargetType:\n");
        if (cast_type)
        {
            cast_type->print_ast(indent + 2);
        }
        print_blank(indent + 1);
        printf("Operand:\n");
        if (operand)
        {
            operand->print_ast(indent + 2);
        }
    }

#pragma endregion // PRINT_AST

#pragma region Evaluate Functions

    String evaluate_expr_result::to_string() const
    {
        switch (result_type)
        {
            case enum_eval_result_type::constant_bool: return bool_value ? "true" : "false";
            case enum_eval_result_type::constant_int: return std::to_string(int_value);
            case enum_eval_result_type::constant_float: return std::to_string(float_value);
            default: return "";
        }
    }

    evaluate_expr_result ast_node_expression::evaluate(shader_codegen_state& state)
    {
        evaluate_expr_result result;
        result.result_type = enum_eval_result_type::undetermined;
        return result;
    }

    evaluate_expr_result binary_expression::evaluate(shader_codegen_state& state)
    {
        if (!left || !right)
        {
            return {};
        }

        const evaluate_expr_result left_result = left->evaluate(state);
        const evaluate_expr_result right_result = right->evaluate(state);

        if (left_result.result_type == enum_eval_result_type::undetermined ||
            right_result.result_type == enum_eval_result_type::undetermined)
        {
            return {};
        }

        // Booleans.
        if (left_result.result_type == enum_eval_result_type::constant_bool &&
            right_result.result_type == enum_eval_result_type::constant_bool)
        {
            switch (op)
            {
            case enum_binary_op::add:
                return { (left_result.bool_value ? 1 : 0) + (right_result.bool_value ? 1 : 0) };
            case enum_binary_op::sub:
                return { (left_result.bool_value ? 1 : 0) - (right_result.bool_value ? 1 : 0) };
            case enum_binary_op::mul:
                return { (left_result.bool_value ? 1 : 0) * (right_result.bool_value ? 1 : 0) };
            case enum_binary_op::div:
                if (right_result.bool_value)
                {
                    return { (left_result.bool_value ? 1 : 0) };
                }
                else
                {
                    return {}; // Divide by zero.
                }
            case enum_binary_op::equal:
                return { left_result.bool_value == right_result.bool_value };
            case enum_binary_op::not_equal:
                return { left_result.bool_value != right_result.bool_value };
            case enum_binary_op::less:
                return { (left_result.bool_value ? 1 : 0) < (right_result.bool_value ? 1 : 0) };
            case enum_binary_op::less_equal:
                return { (left_result.bool_value ? 1 : 0) <= (right_result.bool_value ? 1 : 0) };
            case enum_binary_op::greater:
                return { (left_result.bool_value ? 1 : 0) > (right_result.bool_value ? 1 : 0) };
            case enum_binary_op::greater_equal:
                return { (left_result.bool_value ? 1 : 0) >= (right_result.bool_value ? 1 : 0) };
            case enum_binary_op::logical_and:
                return { left_result.bool_value && right_result.bool_value };
            case enum_binary_op::logical_or:
                return { left_result.bool_value || right_result.bool_value };
            case enum_binary_op::undefined:
                return {};
            }
        }

        if (left_result.result_type == enum_eval_result_type::constant_int &&
            right_result.result_type == enum_eval_result_type::constant_int)
        {
            int result_value = 0;
            bool is_comparison = false;
            
            switch (op)
            {
            case enum_binary_op::add:
                result_value = left_result.int_value + right_result.int_value;
                break;
            case enum_binary_op::sub:
                result_value = left_result.int_value - right_result.int_value;
                break;
            case enum_binary_op::mul:
                result_value = left_result.int_value * right_result.int_value;
                break;
            case enum_binary_op::div:
                if (right_result.int_value != 0)
                    result_value = left_result.int_value / right_result.int_value;
                else
                    return {}; // Devide by zero
                break;
            case enum_binary_op::equal:
                result_value = (left_result.int_value == right_result.int_value) ? 1 : 0;
                is_comparison = true;
                break;
            case enum_binary_op::not_equal:
                result_value = (left_result.int_value != right_result.int_value) ? 1 : 0;
                is_comparison = true;
                break;
            case enum_binary_op::less:
                result_value = (left_result.int_value < right_result.int_value) ? 1 : 0;
                is_comparison = true;
                break;
            case enum_binary_op::less_equal:
                result_value = (left_result.int_value <= right_result.int_value) ? 1 : 0;
                is_comparison = true;
                break;
            case enum_binary_op::greater:
                result_value = (left_result.int_value > right_result.int_value) ? 1 : 0;
                is_comparison = true;
                break;
            case enum_binary_op::greater_equal:
                result_value = (left_result.int_value >= right_result.int_value) ? 1 : 0;
                is_comparison = true;
                break;
            case enum_binary_op::logical_and:
                result_value = (left_result.int_value != 0 && right_result.int_value != 0) ? 1 : 0;
                is_comparison = true;
                break;
            case enum_binary_op::logical_or:
                result_value = (left_result.int_value != 0 || right_result.int_value != 0) ? 1 : 0;
                is_comparison = true;
                break;
            case enum_binary_op::undefined: return {};
            }
            
            if (is_comparison)
                return {static_cast<bool>(result_value)};
            else
                return {result_value};
        }

        if (left_result.result_type == enum_eval_result_type::constant_float &&
            right_result.result_type == enum_eval_result_type::constant_float)
        {
            float result_value;
            
            switch (op)
            {
            case enum_binary_op::add:
                result_value = left_result.float_value + right_result.float_value;
                break;
            case enum_binary_op::sub:
                result_value = left_result.float_value - right_result.float_value;
                break;
            case enum_binary_op::mul:
                result_value = left_result.float_value * right_result.float_value;
                break;
            case enum_binary_op::div:
                if (right_result.float_value != 0.0f)
                    result_value = left_result.float_value / right_result.float_value;
                else
                    return {};
                break;
            case enum_binary_op::equal:
                return {left_result.float_value == right_result.float_value};
            case enum_binary_op::not_equal:
                return {left_result.float_value != right_result.float_value};
            case enum_binary_op::less:
                return {left_result.float_value < right_result.float_value};
            case enum_binary_op::less_equal:
                return {left_result.float_value <= right_result.float_value};
            case enum_binary_op::greater:
                return {left_result.float_value > right_result.float_value};
            case enum_binary_op::greater_equal:
                return {left_result.float_value >= right_result.float_value};
            default:
                return {};
            }
            
            return {result_value};
        }

        if ((left_result.result_type == enum_eval_result_type::constant_int && 
             right_result.result_type == enum_eval_result_type::constant_float) ||
            (left_result.result_type == enum_eval_result_type::constant_float && 
             right_result.result_type == enum_eval_result_type::constant_int))
        {
            float left_val = (left_result.result_type == enum_eval_result_type::constant_int) ? 
                             static_cast<float>(left_result.int_value) : left_result.float_value;
            const float right_val = (right_result.result_type == enum_eval_result_type::constant_int) ? 
                                        static_cast<float>(right_result.int_value) : right_result.float_value;
            
            switch (op)
            {
            case enum_binary_op::add:
                return {left_val + right_val};
            case enum_binary_op::sub:
                return {left_val - right_val};
            case enum_binary_op::mul:
                return {left_val * right_val};
            case enum_binary_op::div:
                if (right_val != 0.0f)
                    return {left_val / right_val};
                else
                    return {};
            case enum_binary_op::equal:
                return {left_val == right_val};
            case enum_binary_op::not_equal:
                return {left_val != right_val};
            case enum_binary_op::less:
                return {left_val < right_val};
            case enum_binary_op::less_equal:
                return {left_val <= right_val};
            case enum_binary_op::greater:
                return {left_val > right_val};
            case enum_binary_op::greater_equal:
                return {left_val >= right_val};
            default:
                return {};
            }
        }

        return {};
    }

    evaluate_expr_result reference_expression::evaluate(shader_codegen_state& state)
    {
        if (!target)
        {
            return {};
        }

        // Referencing an expression.
        if (target->node_type == enum_ast_node_type::expression)
        {
            const auto expr = static_cast<ast_node_expression*>(target);
            return expr->evaluate(state);
        }

        // Variants.
        if (target->node_type == enum_ast_node_type::variable)
        {
            const auto variable = static_cast<ast_node_variable*>(target);
            NameHandle variable_name = variable->name;
            auto variant_it = state.variants.find(variable_name);
            if (variant_it != state.variants.end())
            {
                return { variant_it->second };
            }
        }

        // Variables.
        evaluate_expr_result result;
        result.result_type = enum_eval_result_type::variable;
        result.result_node = target;
        return result;
    }

    evaluate_expr_result constant_int_expression::evaluate(shader_codegen_state& state)
    {
        return {value};
    }

    evaluate_expr_result constant_float_expression::evaluate(shader_codegen_state& state)
    {
        return {value};
    }

    evaluate_expr_result constant_bool_expression::evaluate(shader_codegen_state& state)
    {
        return {value};
    }

    evaluate_expr_result function_call_expression::evaluate(shader_codegen_state& state)
    {
        evaluate_expr_result result;
        result.result_type = enum_eval_result_type::undetermined;
        return result;
    }

    evaluate_expr_result method_call_expression::evaluate(shader_codegen_state& state)
    {
        evaluate_expr_result result;
        result.result_type = enum_eval_result_type::undetermined;
        return result;
    }

    evaluate_expr_result constructor_expression::evaluate(shader_codegen_state& state)
    {
        evaluate_expr_result result;
        result.result_type = enum_eval_result_type::undetermined;
        return result;
    }

    evaluate_expr_result assignment_expression::evaluate(shader_codegen_state& state)
    {
        evaluate_expr_result result;
        result.result_type = enum_eval_result_type::undetermined;
        return result;
    }

    evaluate_expr_result conditional_expression::evaluate(shader_codegen_state& state)
    {
        if (!condition)
        {
            return {};
        }

        const evaluate_expr_result cond_result = condition->evaluate(state);

        if (cond_result.result_type == enum_eval_result_type::constant_bool)
        {
            if (cond_result.bool_value)
            {
                return true_expression ? true_expression->evaluate(state) : evaluate_expr_result{};
            }
            else
            {
                return false_expression ? false_expression->evaluate(state) : evaluate_expr_result{};
            }
        }
        else if (cond_result.result_type == enum_eval_result_type::constant_int)
        {
            if (cond_result.int_value != 0)
            {
                return true_expression ? true_expression->evaluate(state) : evaluate_expr_result{};
            }
            else
            {
                return false_expression ? false_expression->evaluate(state) : evaluate_expr_result{};
            }
        }

        evaluate_expr_result result;
        result.result_type = enum_eval_result_type::undetermined;
        return result;
    }

    evaluate_expr_result unary_expression::evaluate(shader_codegen_state& state)
    {
        if (!operand)
        {
            return {};
        }

        const evaluate_expr_result operand_result = operand->evaluate(state);

        if (op == enum_unary_op::pre_inc || op == enum_unary_op::pre_dec ||
            op == enum_unary_op::post_inc || op == enum_unary_op::post_dec)
        {
            evaluate_expr_result result;
            result.result_type = enum_eval_result_type::undetermined;
            return result;
        }

        if (operand_result.result_type == enum_eval_result_type::constant_int)
        {
            switch (op)
            {
            case enum_unary_op::positive:
                return {operand_result.int_value};
            case enum_unary_op::negative:
                return {-operand_result.int_value};
            case enum_unary_op::logical_not:
                return {operand_result.int_value == 0};
            case enum_unary_op::bit_not:
                return {~operand_result.int_value};
            default:
                break;
            }
        }

        if (operand_result.result_type == enum_eval_result_type::constant_float)
        {
            switch (op)
            {
            case enum_unary_op::positive:
                return {operand_result.float_value};
            case enum_unary_op::negative:
                return {-operand_result.float_value};
            case enum_unary_op::logical_not:
                return {operand_result.float_value == 0.0f};
            default:
                break;
            }
        }

        if (operand_result.result_type == enum_eval_result_type::constant_bool)
        {
            switch (op)
            {
            case enum_unary_op::logical_not:
                return {!operand_result.bool_value};
            default:
                break;
            }
        }

        evaluate_expr_result result;
        result.result_type = enum_eval_result_type::undetermined;
        return result;
    }

    evaluate_expr_result compound_assignment_expression::evaluate(shader_codegen_state& state)
    {
        evaluate_expr_result result;
        result.result_type = enum_eval_result_type::undetermined;
        return result;
    }

    evaluate_expr_result chain_expression::evaluate(shader_codegen_state& state)
    {
        if (!expressions.empty() && expressions.back())
        {
            return expressions.back()->evaluate(state);
        }
        return {};
    }

    evaluate_expr_result cast_expression::evaluate(shader_codegen_state& state)
    {
        if (!operand)
        {
            return {};
        }

        evaluate_expr_result operand_result = operand->evaluate(state);

        if (cast_type)
        {
            switch (cast_type->basic_type)
            {
            case enum_basic_type::tp_int:
                if (operand_result.result_type == enum_eval_result_type::constant_float)
                {
                    return {static_cast<int>(operand_result.float_value)};
                }
                else if (operand_result.result_type == enum_eval_result_type::constant_bool)
                {
                    return {operand_result.bool_value ? 1 : 0};
                }
                break;
                
            case enum_basic_type::tp_float:
                if (operand_result.result_type == enum_eval_result_type::constant_int)
                {
                    return {static_cast<float>(operand_result.int_value)};
                }
                else if (operand_result.result_type == enum_eval_result_type::constant_bool)
                {
                    return {operand_result.bool_value ? 1.0f : 0.0f};
                }
                break;
                
            case enum_basic_type::tp_bool:
                if (operand_result.result_type == enum_eval_result_type::constant_int)
                {
                    return {operand_result.int_value != 0};
                }
                else if (operand_result.result_type == enum_eval_result_type::constant_float)
                {
                    return {operand_result.float_value != 0.0f};
                }
                break;
                
            default:
                break;
            }
        }

        evaluate_expr_result result;
        result.result_type = enum_eval_result_type::undetermined;
        return result;
    }

#pragma endregion // Evaluate Functions


    //////////////////////////////////////////////////////////////////////////
    // Tokenize

    String strip_quotes(const char* text) {
        if (text == nullptr) {
            return "";
        }

        size_t len = std::strlen(text);
    
        if (len == 0) {
            return "";
        }

        if (len >= 2) {
            char first = text[0];
            char last = text[len - 1];

            if (first == '"' && last == '"')
            {
                return String(text + 1, len - 2);
            }
        }

        return String(text);
    }

    int tokenize(token_data& t, const parse_location* loc, const char* text, int text_len, int token)
    {
        sl_state->current_text = text;
        memcpy(sl_state->current_location, loc, sizeof(parse_location));
        
        t.first_line = loc->first_line;
        t.first_column = loc->first_column;
        t.first_source = loc->first_source;
        t.last_line = loc->last_line;
        t.last_column = loc->last_column;
        t.last_source = loc->last_source;

        switch (token)
        {
        case TOKEN_IDENTIFIER:
            {
                if (sl_state->is_key_identifier(text))
                {
                    token = KEY_ID;
                    break;
                }
                const ast_node* symbol_node = sl_state->get_global_symbol(text);
                if(symbol_node == nullptr)
                {
                    token = NEW_ID;
                }
                else if (symbol_node->node_type == enum_ast_node_type::type_format ||
                    symbol_node->node_type == enum_ast_node_type::structure)
                {
                    sl_state->output_parse_log("TYPE_ID : " + sl_state->current_text, loc);
                    token = TYPE_ID;
                }
                break;
            }
        default:
            break;
        }
        
        t.token_id = token;
        t.text = strip_quotes(text);
        t.length = text_len;

        return t.token_id;
    }
}