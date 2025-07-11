#pragma optimize("", off)
#include "AstNode.h"
#include "ShaderLang.h"
#include "Assertion.h"
#include "ShaderCompiler.h"
#include "Templates/RefCounting.h"
#include "../Generated/parser.tab.h"

namespace Thunder
{
    shader_lang_state* sl_state = nullptr;

    static void print_blank(int indent) {
        for (int i = 0; i < indent; i++) printf("  ");
    }

    static const char* get_type_name(enum_basic_type type) {
        switch (type)
        {
            case enum_basic_type::tp_int: return "int";
            case enum_basic_type::tp_float: return "float";
            case enum_basic_type::tp_void: return "void";
            case enum_basic_type::undefined: return "unknown";
        }
        return nullptr;
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

#pragma region HLSL

    void ast_node_type::generate_hlsl(String& outResult)
    {
        String out_name = "";
        /*switch (param_type)
        {
        case enum_basic_type::tp_int: out_name = "int"; break;
        case enum_basic_type::tp_float: out_name = "float"; break;
        case enum_basic_type::tp_void: out_name = "void"; break;
        case enum_basic_type::tp_struct: out_name = type_name; break;
        case enum_basic_type::tp_vector: out_name = type_name; break;
        case enum_basic_type::tp_matrix: out_name = type_name; break;
        case enum_basic_type::tp_texture: out_name = type_name; break;
        case enum_basic_type::tp_sampler: out_name = type_name; break;
            
        default: break;
        }*/
        outResult += type_name;
    }

    void ast_node_variable::generate_hlsl(String& outResult)
    {
        type->generate_hlsl(outResult);
        outResult += " " + name.ToString();
        if (type->is_semantic)
        {
            outResult += " : " + semantic.ToString();
        }
    }

    void ast_node_pass::generate_hlsl(String& outResult)
    {
        if (structure != nullptr)
        {
            structure->generate_hlsl(outResult);
        }
        if (stage != nullptr)
        {
            stage->generate_hlsl(outResult);
        }
    }
    
    void ast_node_struct::generate_hlsl(String& outResult)
    {
        outResult += "struct " + type->type_name+ " {\n";
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
        funcSignature += " " + func_name.ToString() + "(" + paramList + ")\n";

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

    void ast_node_var_declaration::generate_hlsl(String& outResult)
    {
        VarDelType->generate_hlsl(outResult);
        outResult += " " + VarName.ToString();
        if (DelExpression != nullptr)
        {
            outResult += " = ";
            DelExpression->generate_hlsl(outResult);
        }
        outResult += ";\n";
        ast_node_statement::generate_hlsl(outResult);
    }

    void ast_node_assignment::generate_hlsl(String& outResult)
    {
        outResult += LhsVar.ToString() + " = ";
        TAssertf(RhsExpression != nullptr, "Assignment rhs is null");
        RhsExpression->generate_hlsl(outResult);
        outResult += ";\n";
        ast_node_statement::generate_hlsl(outResult);
    }

    void ast_node_return::generate_hlsl(String& outResult)
    {
        outResult += "return ";
        if (RetValue != nullptr)
        {
            RetValue->generate_hlsl(outResult);
        }
        outResult += ";\n";
        ast_node_statement::generate_hlsl(outResult);
    }

    void binary_op_expression::generate_hlsl(String& outResult)
    {
        TAssertf(left != nullptr && right != nullptr, "Binary operation left or right is null");
        //outResult += "(";
        left->generate_hlsl(outResult);
        switch (op) {
        case enum_binary_op::add: outResult += " + "; break;
        case enum_binary_op::sub: outResult += " - "; break;
        case enum_binary_op::mul: outResult += " * "; break;
        case enum_binary_op::div: outResult += " / "; break;
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
        outResult += member_text;
    }

    void reference_expression::generate_hlsl(String& outResult)
    {
        outResult += identifier;
    }

#pragma endregion // HLSL

#pragma region PRINT_AST
    void ast_node_type::print_ast(int indent)
    {
        print_blank(indent);
        printf("Type: %s", type_name.c_str());
    }

    void ast_node_variable::print_ast(int indent)
    {
        type->print_ast(indent);
        printf(" Name: %s", name.ToString().c_str());
        if (type->is_semantic)
        {
            printf(" Semantic: %s", semantic.ToString().c_str());
        }
        printf("\n");
    }

    void ast_node_pass::print_ast(int indent)
    {
        print_blank(indent);
        printf("Pass:\n");
        if (structure != nullptr)
        {
            structure->print_ast(indent + 1);
        }
        if (stage != nullptr)
        {
            stage->print_ast(indent + 1);
        }
    }

    void ast_node_struct::print_ast(int indent)
    {
        print_blank(indent);
        printf("Struct: %s\n", type->type_name.c_str());
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

    void ast_node_var_declaration::print_ast(int indent)
    {
        print_blank(indent);
        printf("VarDecl: {\n");
        VarDelType->print_ast(indent + 1);
        printf("; Name : %s\n", VarName.c_str());
        if (DelExpression != nullptr)
        {
            DelExpression->print_ast(indent + 1);
        }
        print_blank(indent);
        printf("}\n");
        ast_node_statement::print_ast(indent);
    }

    void ast_node_assignment::print_ast(int indent)
    {
        print_blank(indent);
        printf("Assignment: {\n");
        print_blank(indent + 1);
        printf("%s = \n", LhsVar.c_str());
        TAssertf(RhsExpression != nullptr, "Assignment rhs is null");
        RhsExpression->print_ast(indent + 1);
        print_blank(indent);
        printf("}\n");
        ast_node_statement::print_ast(indent);
    }

    void ast_node_return::print_ast(int indent)
    {
        print_blank(indent);
        printf("return: { \n");
        if (RetValue != nullptr)
        {
            RetValue->print_ast(indent + 1);
        }
        else
        {
            printf(";\n");
        }
        print_blank(indent);
        printf("}\n");
        ast_node_statement::print_ast(indent);
    }

    void binary_op_expression::print_ast(int indent)
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
        printf(".%s\n", member_text.c_str());
        print_blank(indent);
        printf("}\n");
    }

    void reference_expression::print_ast(int indent)
    {
        print_blank(indent);
        printf("%s\n", identifier.c_str());
    }

#pragma endregion // PRINT_AST

#pragma region CREATE_AST_NODE
    
    ast_node* create_type_node(const token_data& type_info) {
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
                TAssert(sl_state->get_symbol_type(type_info.text) == enum_symbol_type::type);
            if (auto symbol_node = static_cast<ast_node_type*>(sl_state->get_symbol_node(type_info.text)))
            {
                switch (symbol_node->param_type)
                {
                case enum_basic_type::tp_struct:
                    node->param_type = enum_basic_type::tp_struct;
                    break;
                default:
                    break;
                }
            }
            break;
        }
        default:
            break;
        }

        return node;
    }

    ast_node* create_pass_node(ast_node* struct_node, ast_node* stage_node)
    {
        TAssertf(struct_node != nullptr && struct_node->Type == enum_ast_node_type::structure, "Struct owner type is not correct");
        TAssertf(stage_node != nullptr && stage_node->Type == enum_ast_node_type::function, "Stage owner type is not correct");
        const auto node = new ast_node_pass;
        node->structure = static_cast<ast_node_struct*>(struct_node);
        node->stage = static_cast<ast_node_function*>(stage_node);
        return node;
    }

#pragma endregion // CREATE_AST_NODE

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
            ast_node* symbol_node = sl_state->get_symbol_node(text);
            if(symbol_node == nullptr)
            {
                token = NEW_ID;
            }
            else if (symbol_node->Type == enum_ast_node_type::type)
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

    void post_process_ast(ast_node* nodeRoot)
    {
        if (nodeRoot == nullptr)
        {
            sl_state->debug_log("Parse Error, current text : " + sl_state->current_text);
            return;
        }
        nodeRoot->print_ast(0);
        printf("\n");
        String outHlsl;
        nodeRoot->generate_hlsl(outHlsl);
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