#pragma optimize("", off)
#include "AstNode.h"
#include "Assertion.h"
#include "ShaderCompiler.h"
#include "Templates/RefCounting.h"

namespace Thunder
{

    static void print_blank(int indent) {
        for (int i = 0; i < indent; i++) printf("  ");
    }

    static const char* get_type_name(var_type type) {
        switch (type)
        {
            case var_type::tp_int: return "int";
            case var_type::tp_float: return "float";
            case var_type::tp_void: return "void";
            case var_type::undefined: return "unknown";
        }
        return nullptr;
    }

    void ast_node::GenerateDXIL()
    {
        // ASTNodeAdd
        /*mLeft.GenerateDXIL();
        mRight.GenerateDXIL();
        std::string outResult += "LD EAX " + mLeft.Result();
        outResult += "LD EBX " + mRight.Result();
        outResult += "ADD EAX EBX";*/
    }

#pragma region HLSL

    void ast_node_function::GenerateHLSL(std::string& outResult)
    {
        String funcSignature;
        Signature->GenerateHLSL(funcSignature);
        String funcBody;
        Body->GenerateHLSL(funcBody);
        outResult += funcSignature + "{\n" + funcBody + "}\n";
    }

    void ast_node_func_signature::GenerateHLSL(String& outResult)
    {
        String paramList;
        if (Params != nullptr)
        {
            Params->GenerateHLSL(paramList);
        }
        ReturnType->GenerateHLSL(outResult);
        outResult += " " + FuncName.ToString() + "(" + paramList + ")\n";
    }

    void ast_node_param_list::GenerateHLSL(String& outResult)
    {
        if (ParamsHead != nullptr)
        {
            ParamsHead->GenerateHLSL(outResult);
        }
    }

    void ast_node_param::GenerateHLSL(String& outResult)
    {
        ParamType->GenerateHLSL(outResult);
        outResult += " " + ParamName.ToString();
        if (NextParam != nullptr)
        {
            outResult += ", ";
            NextParam->GenerateHLSL(outResult);
        }
    }

    void ast_node_statement_list::GenerateHLSL(String& outResult)
    {
        if (StatementsHead != nullptr)
        {
            StatementsHead->GenerateHLSL(outResult);
        }
    }

    void ast_node_statement::GenerateHLSL(String& outResult)
    {
        if (NextStatement != nullptr)
        {
            NextStatement->GenerateHLSL(outResult);
        }
    }

    void ast_node_var_declaration::GenerateHLSL(String& outResult)
    {
        VarDelType->GenerateHLSL(outResult);
        outResult += " " + VarName.ToString();
        if (DelExpression != nullptr)
        {
            outResult += " = ";
            DelExpression->GenerateHLSL(outResult);
        }
        outResult += ";\n";
        ast_node_statement::GenerateHLSL(outResult);
    }

    void ast_node_assignment::GenerateHLSL(String& outResult)
    {
        outResult += LhsVar.ToString() + " = ";
        TAssertf(RhsExpression != nullptr, "Assignment rhs is null");
        RhsExpression->GenerateHLSL(outResult);
        outResult += ";\n";
        ast_node_statement::GenerateHLSL(outResult);
    }

    void ast_node_return::GenerateHLSL(String& outResult)
    {
        outResult += "return ";
        if (RetValue != nullptr)
        {
            RetValue->GenerateHLSL(outResult);
        }
        outResult += ";\n";
        ast_node_statement::GenerateHLSL(outResult);
    }

    void ast_node_binary_operation::GenerateHLSL(String& outResult)
    {
        TAssertf(left != nullptr && right != nullptr, "Binary operation left or right is null");
        //outResult += "(";
        left->GenerateHLSL(outResult);
        switch (op) {
        case binary_op::add: outResult += " + "; break;
        case binary_op::sub: outResult += " - "; break;
        case binary_op::mul: outResult += " * "; break;
        case binary_op::div: outResult += " / "; break;
        case binary_op::undefined: break;
        }
        right->GenerateHLSL(outResult);
        //outResult += ")";
    }

    void ast_node_priority::GenerateHLSL(String& outResult)
    {
        outResult += "( ";
        if (Content != nullptr)
        {
            Content->GenerateHLSL(outResult);
        }
        outResult += " )";
    }

    void ast_node_identifier::GenerateHLSL(String& outResult)
    {
        outResult += Identifier.ToString();
    }

    void ast_node_integer::GenerateHLSL(String& outResult)
    {
        outResult += std::to_string(IntValue);
    }

    void ast_node_type::GenerateHLSL(String& outResult)
    {
        outResult += get_type_name(ParamType);
    }

    #pragma endregion

    #pragma region PRINT_AST

    void ast_node_function::PrintAST(int indent)
    {
        print_blank(indent);
        printf("Function:\n");
        Signature->PrintAST(indent + 1);
        Body->PrintAST(indent + 1);
    }

    void ast_node_func_signature::PrintAST(int indent)
    {
        print_blank(indent);
        printf("Signature:\n");
        print_blank(indent + 1);
        printf("FuncName: %s\n", FuncName.c_str());
        print_blank(indent + 1);
        printf("Return");
        ReturnType->PrintAST(0);
        printf("\n");
        Params->PrintAST(indent + 1);
    }

    void ast_node_param_list::PrintAST(int indent)
    {
        print_blank(indent);
        printf("ParamList(Count %d):\n", ParamCount);
        if (ParamsHead != nullptr)
        {
            ParamsHead->PrintAST(indent + 1);
        }
    }

    void ast_node_param::PrintAST(int indent)
    {
        print_blank(indent);
        printf("Param { ");
        ParamType->PrintAST(0);
        printf("; Name : %s }\n", ParamName.c_str());
        if (NextParam != nullptr) {
            NextParam->PrintAST(indent);
        }
    }

    void ast_node_statement_list::PrintAST(int indent)
    {
        print_blank(indent);
        printf("StatementList:\n");
        StatementsHead->PrintAST(indent + 1);
    }

    void ast_node_statement::PrintAST(int indent)
    {
        if (NextStatement != nullptr)
        {
            NextStatement->PrintAST(indent);
        }
    }

    void ast_node_var_declaration::PrintAST(int indent)
    {
        print_blank(indent);
        printf("VarDecl: {\n");
        VarDelType->PrintAST(indent + 1);
        printf("; Name : %s\n", VarName.c_str());
        if (DelExpression != nullptr)
        {
            DelExpression->PrintAST(indent + 1);
        }
        print_blank(indent);
        printf("}\n");
        ast_node_statement::PrintAST(indent);
    }

    void ast_node_assignment::PrintAST(int indent)
    {
        print_blank(indent);
        printf("Assignment: {\n");
        print_blank(indent + 1);
        printf("%s = \n", LhsVar.c_str());
        TAssertf(RhsExpression != nullptr, "Assignment rhs is null");
        RhsExpression->PrintAST(indent + 1);
        print_blank(indent);
        printf("}\n");
        ast_node_statement::PrintAST(indent);
    }

    void ast_node_return::PrintAST(int indent)
    {
        print_blank(indent);
        printf("return: { \n");
        if (RetValue != nullptr)
        {
            RetValue->PrintAST(indent + 1);
        }
        else
        {
            printf(";\n");
        }
        print_blank(indent);
        printf("}\n");
        ast_node_statement::PrintAST(indent);
    }

    void ast_node_binary_operation::PrintAST(int indent)
    {
        TAssertf(left != nullptr && right != nullptr, "Binary operation left or right is null");
        print_blank(indent);
        printf("BinaryOp: ");
        switch (op) {
        case binary_op::add: printf("+\n"); break;
        case binary_op::sub: printf("-\n"); break;
        case binary_op::mul: printf("*\n"); break;
        case binary_op::div: printf("/\n"); break;
        case binary_op::undefined: break;
        }
        left->PrintAST(indent + 1);
        right->PrintAST(indent + 1);
    }

    void ast_node_priority::PrintAST(int indent)
    {
        print_blank(indent);
        printf("Priority: (\n");
        if (Content != nullptr)
        {
            Content->PrintAST(indent + 1);
        }
        print_blank(indent);
        printf(")\n");
    }

    void ast_node_identifier::PrintAST(int indent)
    {
        print_blank(indent);
        printf("IdentifierName: %s\n", Identifier.c_str());
    }

    void ast_node_integer::PrintAST(int indent)
    {
        print_blank(indent);
        printf("Integer: %d\n", IntValue);
    }

    void ast_node_type::PrintAST(int indent)
    {
        print_blank(indent);
        printf("Type: %s", get_type_name(ParamType));
    }

#pragma endregion

#pragma region CREATE_AST_NODE

    ast_node* create_function_node(ast_node* signature, ast_node* body)
    {
        TAssertf(signature != nullptr && signature->Type == basal_ast_node_type::func_signature, "Signature node type is not correct");
        TAssertf(body != nullptr && body->Type == basal_ast_node_type::statement_list, "Body node type is not correct");

        const auto node = new ast_node_function;
        node->Signature = static_cast<ast_node_func_signature*>(signature);
        node->Body = static_cast<ast_node_statement_list*>(body);
        return node;
    }

    ast_node* create_func_signature_node(ast_node* returnTypeNode, const char *name, ast_node* params)
    {
        TAssertf(returnTypeNode != nullptr && returnTypeNode->Type == basal_ast_node_type::type, "Return type node type is not correct");
        TAssertf(params != nullptr && params->Type == basal_ast_node_type::param_list, "Params node type is not correct");

        const auto node = new ast_node_func_signature;
        node->ReturnType = static_cast<ast_node_type*>(returnTypeNode);
        node->FuncName = name;
        node->Params = static_cast<ast_node_param_list*>(params);
        return node;
    }

    ast_node* create_param_list_node()
    {
        const auto node = new ast_node_param_list;
        node->ParamCount = 0;
        node->ParamsHead = nullptr;
        node->ParamsTail = nullptr;
        return node;
    }

    void add_param_to_list(ast_node* list, ast_node* param)
    {
        TAssertf(list != nullptr && list->Type == basal_ast_node_type::param_list, "List node type is not correct");
        TAssertf(param != nullptr && param->Type == basal_ast_node_type::param, "Param node type is not correct");

        if (const auto paramList = static_cast<ast_node_param_list*>(list))
        {
            if (paramList->ParamCount == 0)
            {
                paramList->ParamsHead = static_cast<ast_node_param*>(param);
                paramList->ParamsTail = static_cast<ast_node_param*>(param);
            } else {
                paramList->ParamsTail->NextParam = static_cast<ast_node_param*>(param);
                paramList->ParamsTail = static_cast<ast_node_param*>(param);
            }
            paramList->ParamCount++;
        }
    }

    ast_node* create_param_node(ast_node* typeNode, const char *name)
    {
        TAssertf(typeNode != nullptr && typeNode->Type == basal_ast_node_type::type, "Type node type is not correct");

        const auto node = new ast_node_param;
        node->ParamType = static_cast<ast_node_type*>(typeNode);
        node->ParamName = name;
        node->NextParam = nullptr;
        return node;
    }

    ast_node* create_statement_list_node()
    {
        const auto node = new ast_node_statement_list;
        node->StatementsHead = nullptr;
        node->StatementsTail = nullptr;
        return node;
    }

    void add_statement_to_list(ast_node* list, ast_node* stmt)
    {
        TAssertf(list != nullptr && list->Type == basal_ast_node_type::statement_list, "List node type is not correct");
        TAssertf(stmt != nullptr && stmt->Type == basal_ast_node_type::statement, "Statement node type is not correct");

        if (const auto stmtList = static_cast<ast_node_statement_list*>(list); const auto newStmt = static_cast<ast_node_statement*>(stmt))
        {
            if (stmtList->StatementsHead == nullptr && stmtList->StatementsTail == nullptr) {
                stmtList->StatementsHead = newStmt;
                stmtList->StatementsTail = newStmt;
            } else {
                stmtList->StatementsTail->NextStatement = newStmt;
                stmtList->StatementsTail = newStmt;
            }
        }
    }

    ast_node* create_var_decl_node(ast_node* typeNode, const char *name, ast_node* init_expr)
    {
        TAssertf(typeNode != nullptr && typeNode->Type == basal_ast_node_type::type, "Type node type is not correct");

        const auto node = new ast_node_var_declaration;
        node->VarDelType = static_cast<ast_node_type*>(typeNode);
        node->VarName = name;
        if (init_expr != nullptr)
        {
            TAssertf(init_expr->Type == basal_ast_node_type::expression, "Init expression node type is not correct");
            node->DelExpression = static_cast<ast_node_expression*>(init_expr);
        }
        return node;
    }

    ast_node* create_assignment_node(const char *lhs, ast_node* rhs)
    {
        TAssertf(rhs != nullptr && rhs->Type == basal_ast_node_type::expression, "Assignment rhs is null");

        const auto node = new ast_node_assignment;
        node->LhsVar = lhs;
        node->RhsExpression = static_cast<ast_node_expression*>(rhs);
        return node;
    }

    ast_node* create_return_node(ast_node* expr)
    {
        TAssertf(expr != nullptr && expr->Type == basal_ast_node_type::expression, "Return expression is null");
        const auto node = new ast_node_return;
        node->RetValue = static_cast<ast_node_expression*>(expr);
        return node;
    }

    ast_node* create_binary_op_node(binary_op op, ast_node* left, ast_node* right)
    {
        TAssertf(left != nullptr && left->Type == basal_ast_node_type::expression, "Binary operation left is null");
        TAssertf(right != nullptr && right->Type == basal_ast_node_type::expression, "Binary operation right is null");

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
            TAssert(expr->Type == basal_ast_node_type::expression);
            node->Content = static_cast<ast_node_expression*>(expr);
        }
        return node;
    }

    ast_node* create_type_node(var_type type) {
        const auto node = new ast_node_type;
        node->ParamType = type; // 复用param_type字段
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
        nodeRoot->PrintAST(0);
        printf("\n");
        String outHlsl;
        nodeRoot->GenerateHLSL(outHlsl);
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