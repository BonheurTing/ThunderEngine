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
        // ASTNodeAdd
        /*mLeft.generate_dxil();
        mRight.generate_dxil();
        std::string outResult += "LD EAX " + mLeft.Result();
        outResult += "LD EBX " + mRight.Result();
        outResult += "ADD EAX EBX";*/
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

    String ast_node_type_format::get_type_text() const
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
                ast_node* node = sl_state->custom_types[indirect_index];
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

    void ast_node_type_format::generate_hlsl(String& outResult)
    {
        // 生成类型限定符
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

        if (object_type != enum_object_type::none)
        {
            outResult += TemplateTypeNames[static_cast<size_t>(object_type)];
            outResult += '<';
            outResult += get_type_text();
            outResult += '>';
        }
        else
        {
            outResult += get_type_text();
        }
    }

    void ast_node_variable::generate_hlsl(String& outResult)
    {
        type->generate_hlsl(outResult);
        outResult += " " + name;
        if (type->is_semantic)
        {
            outResult += " : " + semantic;
        }
    }

    void ast_node_archive::generate_hlsl(String& outResult)
    {
        TArray<ast_node_variable*> objects;
        if (!object_parameters.empty())
        {
            outResult += "cbuffer cbObject : register(b0, space0)\n{\n";
            for (const auto& param : object_parameters)
            {
                if (param->type->is_object())
                {
                    objects.push_back(param);
                    continue;
                }
                param->generate_hlsl(outResult);
                outResult += ";\n";
            }
            outResult += "};\n\n";
        }
        if (!pass_parameters.empty())
        {
            outResult += "cbuffer cbPass : register(b1, space0)\n{\n";
            for (const auto& param : pass_parameters)
            {
                if (param->type->is_object())
                {
                    objects.push_back(param);
                    continue;
                }
                param->generate_hlsl(outResult);
                outResult += ";\n";
            }
            outResult += "};\n\n";
        }
        if (!global_parameters.empty())
        {
            outResult += "cbuffer cbGlobal : register(b2, space0)\n{\n";
            for (const auto& param : global_parameters)
            {
                if (param->type->is_object())
                {
                    objects.push_back(param);
                    continue;
                }
                param->generate_hlsl(outResult);
                outResult += ";\n";
            }
            outResult += "};\n\n";
        }
        int object_index = 0;
        for (const auto& obj : objects)
        {
            obj->type->generate_hlsl(outResult);
            outResult += " " + obj->name + " : register(t" + std::to_string(object_index++) + ");\n";
        }
        outResult += "\n";

        for (const auto pass : passes)
        {
            pass->generate_hlsl(outResult);
            outResult += "\n";
        }
    }

    void ast_node_pass::generate_hlsl(String& outResult)
    {
        for (const auto& def : structures)
        {
            def->generate_hlsl(outResult);
            outResult += "\n";
        }
        for (const auto& def : functions)
        {
            def->generate_hlsl(outResult);
            outResult += "\n";
        }
    }
    
    void ast_node_struct::generate_hlsl(String& outResult)
    {
        outResult += "struct " + name + " {\n";
        for (const auto& member : members)
        {
            member->generate_hlsl(outResult);
            outResult += ";\n";
        }
        outResult += "};\n";
    }

    void ast_node_function::generate_hlsl(std::string& outResult)
    {
        String paramList;
        for (const auto& param : params)
        {
            if (!paramList.empty())
            {
                paramList += ", ";
            }
            param->generate_hlsl(paramList);
        }

        String funcSignature;
        return_type->generate_hlsl(funcSignature);
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
        body->generate_hlsl(funcBody);
        outResult += funcSignature + "{\n" + funcBody + "}\n";
    }

    void ast_node_block::generate_hlsl(String& outResult)
    {
        for(const auto stat : statements)
        {
            stat->generate_hlsl(outResult);
        }
    }

    void variable_declaration_statement::generate_hlsl(String& outResult)
    {
        variable->generate_hlsl(outResult);
        
        if (decl_expr)
        {
            outResult += " = ";
            decl_expr->generate_hlsl(outResult);
        }
        
        outResult += ";\n";
    }

    void return_statement::generate_hlsl(String& outResult)
    {
        outResult += "return ";
        if (ret_value != nullptr)
        {
            ret_value->generate_hlsl(outResult);
        }
        outResult += ";\n";
    }

    void break_statement::generate_hlsl(String& outResult)
    {
        outResult += "break;\n";
    }

    void continue_statement::generate_hlsl(String& outResult)
    {
        outResult += "continue;\n";
    }

    void discard_statement::generate_hlsl(String& outResult)
    {
        outResult += "discard;\n";
    }

    void expression_statement::generate_hlsl(String& outResult)
    {
        if (expr != nullptr)
        {
            expr->generate_hlsl(outResult);
            outResult += ";\n";
        }
    }

    void condition_statement::generate_hlsl(String& outResult)
    {
        if (!condition)
            return;

        const auto cond_ret = condition->evaluate();
        if (cond_ret.result_type == enum_eval_result_type::constant_bool)
        {
            if (cond_ret.bool_value)
            {
                if (true_branch)
                {
                    true_branch->generate_hlsl(outResult);
                }
            }
            else
            {
                if (false_branch)
                {
                    false_branch->generate_hlsl(outResult);
                }
            }
        }
        else if (cond_ret.result_type == enum_eval_result_type::constant_int)
        {
            if (cond_ret.int_value > 0)
            {
                if (true_branch)
                {
                    true_branch->generate_hlsl(outResult);
                }
            }
            else
            {
                if (false_branch)
                {
                    false_branch->generate_hlsl(outResult);
                }
            }
        }
        else if (cond_ret.result_type == enum_eval_result_type::constant_float)
        {
            if (cond_ret.float_value > 0.0f)
            {
                if (true_branch)
                {
                    true_branch->generate_hlsl(outResult);
                }
            }
            else
            {
                if (false_branch)
                {
                    false_branch->generate_hlsl(outResult);
                }
            }
        }
        else
        {
            if (true_branch)
            {
                outResult += "if (";
                condition->generate_hlsl(outResult);
                outResult += ") {\n";
                true_branch->generate_hlsl(outResult);
                outResult += "}\n";
            }
            if (false_branch)
            {
                outResult += "else {\n";
                false_branch->generate_hlsl(outResult);
                outResult += "}\n";
            }
        }
    }

    void binary_expression::generate_hlsl(String& outResult)
    {
        TAssertf(left != nullptr && right != nullptr, "Binary operation left or right is null");
        //outResult += "(";
        left->generate_hlsl(outResult);
        switch (op) {
        case enum_binary_op::add: outResult += " + "; break;
        case enum_binary_op::sub: outResult += " - "; break;
        case enum_binary_op::mul: outResult += " * "; break;
        case enum_binary_op::div: outResult += " / "; break;
        case enum_binary_op::mod: outResult += " % "; break;
        // 位运算
        case enum_binary_op::bit_and: outResult += " & "; break;
        case enum_binary_op::bit_or: outResult += " | "; break;
        case enum_binary_op::bit_xor: outResult += " ^ "; break;
        case enum_binary_op::left_shift: outResult += " << "; break;
        case enum_binary_op::right_shift: outResult += " >> "; break;
        // 逻辑运算
        case enum_binary_op::logical_and: outResult += " && "; break;
        case enum_binary_op::logical_or: outResult += " || "; break;
        // 比较运算
        case enum_binary_op::equal: outResult += " == "; break;
        case enum_binary_op::not_equal: outResult += " != "; break;
        case enum_binary_op::less: outResult += " < "; break;
        case enum_binary_op::less_equal: outResult += " <= "; break;
        case enum_binary_op::greater: outResult += " > "; break;
        case enum_binary_op::greater_equal: outResult += " >= "; break;
        case enum_binary_op::undefined: break;
        }
        right->generate_hlsl(outResult);
        //outResult += ")";
    }

    void shuffle_expression::generate_hlsl(String& outResult)
    {
        prefix->generate_hlsl(outResult);
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

    void component_expression::generate_hlsl(String& outResult)
    {
        component_name->generate_hlsl(outResult);
        outResult += ".";
        outResult += identifier;
    }

    void index_expression::generate_hlsl(String& outResult)
    {
        target->generate_hlsl(outResult);
        outResult += "[";
        if (index)
        {
            index->generate_hlsl(outResult);
        }
        outResult += "]";
    }

    void reference_expression::generate_hlsl(String& outResult)
    {
        outResult += identifier;
    }

    void constant_int_expression::generate_hlsl(String& outResult)
    {
        outResult += std::to_string(value);
    }

    void constant_float_expression::generate_hlsl(String& outResult)
    {
        outResult += std::to_string(value) + "f";
    }

    void constant_bool_expression::generate_hlsl(String& outResult)
    {
        outResult += value ? "true" : "false";
    }

    void for_statement::generate_hlsl(String& outResult)
    {
        outResult += "for (";
        if (init_stmt)
        {
            String init_str;
            init_stmt->generate_hlsl(init_str);
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
            condition->generate_hlsl(outResult);
        }
        outResult += "; ";
        
        if (update_expr)
        {
            update_expr->generate_hlsl(outResult);
        }
        outResult += ") {\n";
        
        if (body_stmt)
        {
            body_stmt->generate_hlsl(outResult);
        }
        outResult += "}\n";
    }

    void function_call_expression::generate_hlsl(String& outResult)
    {
        outResult += function_name + "(";
        String arguments_string;
        for (const auto argument : arguments)
        {
            if (arguments_string.length() > 0)
            {
                arguments_string += ", ";
            }
            argument->generate_hlsl(arguments_string);
        }
        outResult += arguments_string;
        outResult += ")";
    }

    void constructor_expression::generate_hlsl(String& outResult)
    {
        if (constructor_type)
        {
            constructor_type->generate_hlsl(outResult);
        }
        outResult += "(";
        String arguments_string;
        for (const auto argument : arguments)
        {
            if (arguments_string.length() > 0)
            {
                arguments_string += ", ";
            }
            argument->generate_hlsl(arguments_string);
        }
        outResult += arguments_string;
        outResult += ")";
    }

    void assignment_expression::generate_hlsl(String& outResult)
    {
        if (left_expr)
        {
            left_expr->generate_hlsl(outResult);
        }
        outResult += " = ";
        if (right_expr)
        {
            right_expr->generate_hlsl(outResult);
        }
    }

    void conditional_expression::generate_hlsl(String& outResult)
    {
        if (condition)
        {
            condition->generate_hlsl(outResult);
        }
        outResult += " ? ";
        if (true_expression)
        {
            true_expression->generate_hlsl(outResult);
        }
        outResult += " : ";
        if (false_expression)
        {
            false_expression->generate_hlsl(outResult);
        }
    }

    void unary_expression::generate_hlsl(String& outResult)
    {
        switch (op) {
        case enum_unary_op::pre_inc:
            outResult += "++";
            if (operand) operand->generate_hlsl(outResult);
            break;
        case enum_unary_op::pre_dec:
            outResult += "--";
            if (operand) operand->generate_hlsl(outResult);
            break;
        case enum_unary_op::post_inc:
            if (operand) operand->generate_hlsl(outResult);
            outResult += "++";
            break;
        case enum_unary_op::post_dec:
            if (operand) operand->generate_hlsl(outResult);
            outResult += "--";
            break;
        case enum_unary_op::positive:
            outResult += "+";
            if (operand) operand->generate_hlsl(outResult);
            break;
        case enum_unary_op::negative:
            outResult += "-";
            if (operand) operand->generate_hlsl(outResult);
            break;
        case enum_unary_op::logical_not:
            outResult += "!";
            if (operand) operand->generate_hlsl(outResult);
            break;
        case enum_unary_op::bit_not:
            outResult += "~";
            if (operand) operand->generate_hlsl(outResult);
            break;
        case enum_unary_op::undefined:
            break;
        }
    }

    void compound_assignment_expression::generate_hlsl(String& outResult)
    {
        if (left_expr)
        {
            left_expr->generate_hlsl(outResult);
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
            right_expr->generate_hlsl(outResult);
        }
    }

    void chain_expression::generate_hlsl(String& outResult)
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
                expressions[i]->generate_hlsl(outResult);
            }
        }
        outResult += "}";
    }

    void cast_expression::generate_hlsl(String& outResult)
    {
        outResult += "(";
        if (cast_type)
        {
            cast_type->generate_hlsl(outResult);
        }
        outResult += ")";
        if (operand)
        {
            operand->generate_hlsl(outResult);
        }
    }

#pragma endregion // HLSL

#pragma region PRINT_AST
    void ast_node_type_format::print_ast(int indent)
    {
        print_blank(indent);
        String type_text;
        generate_hlsl(type_text);
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
        printf("Pass:\n");
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

    // 基类evaluate函数默认实现
    evaluate_expr_result ast_node_expression::evaluate()
    {
        evaluate_expr_result result;
        result.result_type = enum_eval_result_type::undetermined;
        return result;
    }

    // 二元运算表达式evaluate实现
    evaluate_expr_result binary_expression::evaluate()
    {
        if (!left || !right)
        {
            return {};
        }

        const evaluate_expr_result left_result = left->evaluate();
        const evaluate_expr_result right_result = right->evaluate();

        // 如果任一操作数无法确定，返回未确定结果
        if (left_result.result_type == enum_eval_result_type::undetermined ||
            right_result.result_type == enum_eval_result_type::undetermined)
        {
            return {};
        }

        // 常量折叠 - 整数运算
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
                    return {}; // 除零错误
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

        // 常量折叠 - 浮点数运算
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

        // 混合类型运算（整数和浮点数）
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

        // 无法进行常量折叠，返回未确定结果
        return {};
    }

    // 引用表达式evaluate实现
    evaluate_expr_result reference_expression::evaluate()
    {
        if (!target)
        {
            return {};
        }

        // 如果引用的是常量，尝试获取其值
        if (target->node_type == enum_ast_node_type::expression)
        {
            const auto expr = static_cast<ast_node_expression*>(target);
            return expr->evaluate();
        }

        // 对于变量或其他类型，返回变量类型的结果
        evaluate_expr_result result;
        result.result_type = enum_eval_result_type::variable;
        result.result_node = target;
        return result;
    }

    evaluate_expr_result constant_int_expression::evaluate()
    {
        return {value};
    }

    evaluate_expr_result constant_float_expression::evaluate()
    {
        return {value};
    }

    evaluate_expr_result constant_bool_expression::evaluate()
    {
        return {value};
    }

    evaluate_expr_result function_call_expression::evaluate()
    {
        // 函数调用一般无法在编译时求值，返回未确定结果
        evaluate_expr_result result;
        result.result_type = enum_eval_result_type::undetermined;
        return result;
    }

    evaluate_expr_result constructor_expression::evaluate()
    {
        // 构造函数调用一般无法在编译时求值，返回未确定结果
        evaluate_expr_result result;
        result.result_type = enum_eval_result_type::undetermined;
        return result;
    }

    evaluate_expr_result assignment_expression::evaluate()
    {
        // 赋值表达式一般无法在编译时求值，返回未确定结果
        evaluate_expr_result result;
        result.result_type = enum_eval_result_type::undetermined;
        return result;
    }

    evaluate_expr_result conditional_expression::evaluate()
    {
        if (!condition)
        {
            return {};
        }

        const evaluate_expr_result cond_result = condition->evaluate();
        
        // 如果条件是常量，可以直接选择分支
        if (cond_result.result_type == enum_eval_result_type::constant_bool)
        {
            if (cond_result.bool_value)
            {
                return true_expression ? true_expression->evaluate() : evaluate_expr_result{};
            }
            else
            {
                return false_expression ? false_expression->evaluate() : evaluate_expr_result{};
            }
        }
        else if (cond_result.result_type == enum_eval_result_type::constant_int)
        {
            if (cond_result.int_value != 0)
            {
                return true_expression ? true_expression->evaluate() : evaluate_expr_result{};
            }
            else
            {
                return false_expression ? false_expression->evaluate() : evaluate_expr_result{};
            }
        }
        
        // 条件无法确定，返回未确定结果
        evaluate_expr_result result;
        result.result_type = enum_eval_result_type::undetermined;
        return result;
    }

    evaluate_expr_result unary_expression::evaluate()
    {
        if (!operand)
        {
            return {};
        }

        const evaluate_expr_result operand_result = operand->evaluate();
        
        // 对于 ++ 和 -- 运算符，一般无法在编译时求值
        if (op == enum_unary_op::pre_inc || op == enum_unary_op::pre_dec ||
            op == enum_unary_op::post_inc || op == enum_unary_op::post_dec)
        {
            evaluate_expr_result result;
            result.result_type = enum_eval_result_type::undetermined;
            return result;
        }

        // 常量折叠 - 整数运算
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

        // 常量折叠 - 浮点数运算
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

        // 常量折叠 - 布尔运算
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

        // 无法进行常量折叠，返回未确定结果
        evaluate_expr_result result;
        result.result_type = enum_eval_result_type::undetermined;
        return result;
    }

    evaluate_expr_result compound_assignment_expression::evaluate()
    {
        // 复合赋值表达式一般无法在编译时求值，返回未确定结果
        evaluate_expr_result result;
        result.result_type = enum_eval_result_type::undetermined;
        return result;
    }

    evaluate_expr_result chain_expression::evaluate()
    {
        // 链式表达式返回最后一个表达式的值
        if (!expressions.empty() && expressions.back())
        {
            return expressions.back()->evaluate();
        }
        return {};
    }

    evaluate_expr_result cast_expression::evaluate()
    {
        if (!operand)
        {
            return {};
        }

        evaluate_expr_result operand_result = operand->evaluate();
        
        // 对于常量表达式，尝试进行类型转换
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
        
        // 无法进行常量折叠，返回未确定结果
        evaluate_expr_result result;
        result.result_type = enum_eval_result_type::undetermined;
        return result;
    }

#pragma endregion // Evaluate Functions


    //////////////////////////////////////////////////////////////////////////
    // Tokenize

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
                const ast_node* symbol_node = sl_state->get_global_symbol(text);
                if(symbol_node == nullptr)
                {
                    token = NEW_ID;
                }
                else if (symbol_node->node_type == enum_ast_node_type::type_format ||
                    symbol_node->node_type == enum_ast_node_type::structure)
                {
                    sl_state->debug_log("TYPE_ID : " + sl_state->current_text, loc);
                    token = TYPE_ID;
                }
                break;
            }
        default:
            break;
        }
        
        t.token_id = token;
        t.text = text;
        t.length = text_len;

        return t.token_id;
    }
}