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
        switch (param_type)
        {
        case enum_basic_type::tp_int: out_name = "int"; break;
        case enum_basic_type::tp_float: out_name = "float"; break;
        case enum_basic_type::tp_void: out_name = "void"; break;
        case enum_basic_type::tp_vector: out_name = type_name.ToString(); break;
        case enum_basic_type::tp_matrix: out_name = type_name.ToString(); break;
        case enum_basic_type::tp_texture: out_name = type_name.ToString(); break;
        case enum_basic_type::tp_sampler: out_name = type_name.ToString(); break;
        case enum_basic_type::undefined: break;
        }
        outResult += out_name;
    }

    void ast_node_variable::generate_hlsl(String& outResult)
    {
        type->generate_hlsl(outResult);
        outResult += " " + name.ToString();
        if (type->is_semantic)
        {
            outResult += " : " + semantic.ToString();
        }
        outResult += ";\n";
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
        outResult += "struct " + name.ToString() + " {\n";
        for (const auto& member : members)
        {
            member->generate_hlsl(outResult);
        }
        outResult += "};\n";
    }
    
    void ast_node_function::generate_hlsl(std::string& outResult)
    {
        String funcSignature;
        signature->generate_hlsl(funcSignature);
        String funcBody;
        body->generate_hlsl(funcBody);
        outResult += funcSignature + "{\n" + funcBody + "}\n";
    }

    void ast_node_func_signature::generate_hlsl(String& outResult)
    {
        String paramList;
        if (params != nullptr)
        {
            params->generate_hlsl(paramList);
        }
        return_type->generate_hlsl(outResult);
        outResult += " " + func_name.ToString() + "(" + paramList + ")\n";
    }

    void ast_node_param_list::generate_hlsl(String& outResult)
    {
        if (params_head != nullptr)
        {
            params_head->generate_hlsl(outResult);
        }
    }

    void ast_node_param::generate_hlsl(String& outResult)
    {
        param_type->generate_hlsl(outResult);
        outResult += " " + param_name.ToString();
        if (next_param != nullptr)
        {
            outResult += ", ";
            next_param->generate_hlsl(outResult);
        }
    }

    void ast_node_statement_list::generate_hlsl(String& outResult)
    {
        if (statements_head != nullptr)
        {
            statements_head->generate_hlsl(outResult);
        }
    }

    void ast_node_statement::generate_hlsl(String& outResult)
    {
        if (NextStatement != nullptr)
        {
            NextStatement->generate_hlsl(outResult);
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

    void ast_node_binary_operation::generate_hlsl(String& outResult)
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

    void ast_node_priority::generate_hlsl(String& outResult)
    {
        outResult += "( ";
        if (Content != nullptr)
        {
            Content->generate_hlsl(outResult);
        }
        outResult += " )";
    }

    void ast_node_identifier::generate_hlsl(String& outResult)
    {
        outResult += Identifier.ToString();
    }

    void ast_node_integer::generate_hlsl(String& outResult)
    {
        outResult += std::to_string(IntValue);
    }

#pragma endregion

#pragma region PRINT_AST
    void ast_node_type::print_ast(int indent)
    {
        print_blank(indent);
        printf("Type: %s", get_type_name(param_type));
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
        printf("Struct: %s\n", name.ToString().c_str());
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
        signature->print_ast(indent + 1);
        body->print_ast(indent + 1);
    }

    void ast_node_func_signature::print_ast(int indent)
    {
        print_blank(indent);
        printf("Signature:\n");
        print_blank(indent + 1);
        printf("FuncName: %s\n", func_name.c_str());
        print_blank(indent + 1);
        printf("Return");
        return_type->print_ast(0);
        printf("\n");
        params->print_ast(indent + 1);
    }

    void ast_node_param_list::print_ast(int indent)
    {
        print_blank(indent);
        printf("ParamList(Count %d):\n", param_count);
        if (params_head != nullptr)
        {
            params_head->print_ast(indent + 1);
        }
    }

    void ast_node_param::print_ast(int indent)
    {
        print_blank(indent);
        printf("Param { ");
        param_type->print_ast(0);
        printf("; Name : %s }\n", param_name.c_str());
        if (next_param != nullptr) {
            next_param->print_ast(indent);
        }
    }

    void ast_node_statement_list::print_ast(int indent)
    {
        print_blank(indent);
        printf("StatementList:\n");
        statements_head->print_ast(indent + 1);
    }

    void ast_node_statement::print_ast(int indent)
    {
        if (NextStatement != nullptr)
        {
            NextStatement->print_ast(indent);
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

    void ast_node_binary_operation::print_ast(int indent)
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

    void ast_node_priority::print_ast(int indent)
    {
        print_blank(indent);
        printf("Priority: (\n");
        if (Content != nullptr)
        {
            Content->print_ast(indent + 1);
        }
        print_blank(indent);
        printf(")\n");
    }

    void ast_node_identifier::print_ast(int indent)
    {
        print_blank(indent);
        printf("IdentifierName: %s\n", Identifier.c_str());
    }

    void ast_node_integer::print_ast(int indent)
    {
        print_blank(indent);
        printf("Integer: %d\n", IntValue);
    }

#pragma endregion

    int tokenize(token_data& t, const parse_location* loc, const char* text, int text_len, int token)
    {
        t.token_id = token;
        t.text = text;
        t.length = text_len;

        t.first_line = loc->first_line;
        t.first_column = loc->first_column;
        t.first_source = loc->first_source;
        t.last_line = loc->last_line;
        t.last_column = loc->last_column;
        t.last_source = loc->last_source;

        return t.token_id;
    }

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
        default:
            break;
        }

        return node;
    }

    ast_node* create_pass_node(ast_node* struct_node, ast_node* stage_node)
    {
        TAssertf(struct_node != nullptr && struct_node->Type == enum_ast_node_type::structure, "Struct node type is not correct");
        TAssertf(stage_node != nullptr && stage_node->Type == enum_ast_node_type::function, "Stage node type is not correct");
        const auto node = new ast_node_pass;
        node->structure = static_cast<ast_node_struct*>(struct_node);
        node->stage = static_cast<ast_node_function*>(stage_node);
        return node;
    }

    ast_node* create_function_node(ast_node* signature, ast_node* body)
    {
        TAssertf(signature != nullptr && signature->Type == enum_ast_node_type::func_signature, "Signature node type is not correct");
        TAssertf(body != nullptr && body->Type == enum_ast_node_type::statement_list, "Body node type is not correct");

        const auto node = new ast_node_function;
        node->signature = static_cast<ast_node_func_signature*>(signature);
        node->body = static_cast<ast_node_statement_list*>(body);
        return node;
    }

    ast_node* create_func_signature_node(ast_node* returnTypeNode, const char *name, ast_node* params)
    {
        TAssertf(returnTypeNode != nullptr && returnTypeNode->Type == enum_ast_node_type::type, "Return type node type is not correct");
        TAssertf(params != nullptr && params->Type == enum_ast_node_type::param_list, "Params node type is not correct");

        const auto node = new ast_node_func_signature;
        node->return_type = static_cast<ast_node_type*>(returnTypeNode);
        node->func_name = name;
        node->params = static_cast<ast_node_param_list*>(params);
        return node;
    }

    ast_node* create_param_list_node()
    {
        const auto node = new ast_node_param_list;
        node->param_count = 0;
        node->params_head = nullptr;
        node->params_tail = nullptr;
        return node;
    }

    void add_param_to_list(ast_node* list, ast_node* param)
    {
        TAssertf(list != nullptr && list->Type == enum_ast_node_type::param_list, "List node type is not correct");
        TAssertf(param != nullptr && param->Type == enum_ast_node_type::param, "Param node type is not correct");

        if (const auto paramList = static_cast<ast_node_param_list*>(list))
        {
            if (paramList->param_count == 0)
            {
                paramList->params_head = static_cast<ast_node_param*>(param);
                paramList->params_tail = static_cast<ast_node_param*>(param);
            } else {
                paramList->params_tail->next_param = static_cast<ast_node_param*>(param);
                paramList->params_tail = static_cast<ast_node_param*>(param);
            }
            paramList->param_count++;
        }
    }

    ast_node* create_param_node(ast_node* typeNode, const char *name)
    {
        TAssertf(typeNode != nullptr && typeNode->Type == enum_ast_node_type::type, "Type node type is not correct");

        const auto node = new ast_node_param;
        node->param_type = static_cast<ast_node_type*>(typeNode);
        node->param_name = name;
        node->next_param = nullptr;
        return node;
    }

    ast_node* create_statement_list_node()
    {
        const auto node = new ast_node_statement_list;
        node->statements_head = nullptr;
        node->StatementsTail = nullptr;
        return node;
    }

    void add_statement_to_list(ast_node* list, ast_node* stmt)
    {
        TAssertf(list != nullptr && list->Type == enum_ast_node_type::statement_list, "List node type is not correct");
        TAssertf(stmt != nullptr && stmt->Type == enum_ast_node_type::statement, "Statement node type is not correct");

        if (const auto stmtList = static_cast<ast_node_statement_list*>(list); const auto newStmt = static_cast<ast_node_statement*>(stmt))
        {
            if (stmtList->statements_head == nullptr && stmtList->StatementsTail == nullptr) {
                stmtList->statements_head = newStmt;
                stmtList->StatementsTail = newStmt;
            } else {
                stmtList->StatementsTail->NextStatement = newStmt;
                stmtList->StatementsTail = newStmt;
            }
        }
    }

    ast_node* create_var_decl_node(ast_node* typeNode, const char *name, ast_node* init_expr)
    {
        TAssertf(typeNode != nullptr && typeNode->Type == enum_ast_node_type::type, "Type node type is not correct");

        const auto node = new ast_node_var_declaration;
        node->VarDelType = static_cast<ast_node_type*>(typeNode);
        node->VarName = name;
        if (init_expr != nullptr)
        {
            TAssertf(init_expr->Type == enum_ast_node_type::expression, "Init expression node type is not correct");
            node->DelExpression = static_cast<ast_node_expression*>(init_expr);
        }
        return node;
    }

    ast_node* create_assignment_node(const char *lhs, ast_node* rhs)
    {
        TAssertf(rhs != nullptr && rhs->Type == enum_ast_node_type::expression, "Assignment rhs is null");

        const auto node = new ast_node_assignment;
        node->LhsVar = lhs;
        node->RhsExpression = static_cast<ast_node_expression*>(rhs);
        return node;
    }

    ast_node* create_return_node(ast_node* expr)
    {
        TAssertf(expr != nullptr && expr->Type == enum_ast_node_type::expression, "Return expression is null");
        const auto node = new ast_node_return;
        node->RetValue = static_cast<ast_node_expression*>(expr);
        return node;
    }

    ast_node* create_binary_op_node(enum_binary_op op, ast_node* left, ast_node* right)
    {
        TAssertf(left != nullptr && left->Type == enum_ast_node_type::expression, "Binary operation left is null");
        TAssertf(right != nullptr && right->Type == enum_ast_node_type::expression, "Binary operation right is null");

        const auto node = new ast_node_binary_operation;
        node->op = op;
        node->left = static_cast<ast_node_expression*>(left);
        node->right = static_cast<ast_node_expression*>(right);
        return node;
    }

    ast_node* create_priority_node(ast_node* expr)
    {
        const auto node = new ast_node_priority;
        if (expr)
        {
            TAssert(expr->Type == enum_ast_node_type::expression);
            node->Content = static_cast<ast_node_expression*>(expr);
        }
        return node;
    }

    ast_node* create_identifier_node(const char *name) {
        const auto node = new ast_node_identifier;
        node->Identifier = name;
        return node;
    }

    ast_node* create_int_literal_node(int value) {
        const auto node = new ast_node_integer;
        node->IntValue = value;
        return node;
    }

#pragma endregion

    void post_process_ast(ast_node* nodeRoot)
    {
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