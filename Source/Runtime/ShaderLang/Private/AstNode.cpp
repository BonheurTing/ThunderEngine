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

    static const char* get_type_name(EVarType type) {
        switch (type)
        {
            case EVarType::TP_INT: return "int";
            case EVarType::TP_FLOAT: return "float";
            case EVarType::TP_VOID: return "void";
            case EVarType::TP_UNDEFINED: return "unknown";
        }
        return nullptr;
    }

    void ASTNode::GenerateDXIL()
    {
        // ASTNodeAdd
        /*mLeft.GenerateDXIL();
        mRight.GenerateDXIL();
        std::string outResult += "LD EAX " + mLeft.Result();
        outResult += "LD EBX " + mRight.Result();
        outResult += "ADD EAX EBX";*/
    }

#pragma region HLSL

    void ASTNodeFunction::GenerateHLSL(std::string& outResult)
    {
        String funcSignature;
        Signature->GenerateHLSL(funcSignature);
        String funcBody;
        Body->GenerateHLSL(funcBody);
        outResult += funcSignature + "{\n" + funcBody + "}\n";
    }

    void ASTNodeFuncSignature::GenerateHLSL(String& outResult)
    {
        String paramList;
        if (Params != nullptr)
        {
            Params->GenerateHLSL(paramList);
        }
        ReturnType->GenerateHLSL(outResult);
        outResult += " " + FuncName.ToString() + "(" + paramList + ")\n";
    }

    void ASTNodeParamList::GenerateHLSL(String& outResult)
    {
        if (ParamsHead != nullptr)
        {
            ParamsHead->GenerateHLSL(outResult);
        }
    }

    void ASTNodeParam::GenerateHLSL(String& outResult)
    {
        ParamType->GenerateHLSL(outResult);
        outResult += " " + ParamName.ToString();
        if (NextParam != nullptr)
        {
            outResult += ", ";
            NextParam->GenerateHLSL(outResult);
        }
    }

    void ASTNodeStatementList::GenerateHLSL(String& outResult)
    {
        if (StatementsHead != nullptr)
        {
            StatementsHead->GenerateHLSL(outResult);
        }
    }

    void ASTNodeStatement::GenerateHLSL(String& outResult)
    {
        if (StatContent != nullptr)
        {
            StatContent->GenerateHLSL(outResult);
        }
        if (NextStatement != nullptr)
        {
            NextStatement->GenerateHLSL(outResult);
        }
    }

    void ASTNodeVarDeclaration::GenerateHLSL(String& outResult)
    {
        VarDelType->GenerateHLSL(outResult);
        outResult += " " + VarName.ToString();
        if (DelExpression != nullptr)
        {
            outResult += " = ";
            DelExpression->GenerateHLSL(outResult);
        }
        outResult += ";\n";
    }

    void ASTNodeAssignment::GenerateHLSL(String& outResult)
    {
        outResult += LhsVar.ToString() + " = ";
        TAssertf(RhsExpression != nullptr, "Assignment rhs is null");
        RhsExpression->GenerateHLSL(outResult);
        outResult += ";\n";
    }

    void ASTNodeReturn::GenerateHLSL(String& outResult)
    {
        outResult += "return ";
        if (RetValue != nullptr)
        {
            RetValue->GenerateHLSL(outResult);
        }
        outResult += ";\n";
    }

    void ASTNodeBinaryOperation::GenerateHLSL(String& outResult)
    {
        TAssertf(left != nullptr && right != nullptr, "Binary operation left or right is null");
        //outResult += "(";
        left->GenerateHLSL(outResult);
        switch (op) {
        case EBinaryOp::OP_ADD: outResult += " + "; break;
        case EBinaryOp::OP_SUB: outResult += " - "; break;
        case EBinaryOp::OP_MUL: outResult += " * "; break;
        case EBinaryOp::OP_DIV: outResult += " / "; break;
        case EBinaryOp::OP_UNDEFINED: break;
        }
        right->GenerateHLSL(outResult);
        //outResult += ")";
    }

    void ASTNodePriority::GenerateHLSL(String& outResult)
    {
        outResult += "( ";
        if (Content != nullptr)
        {
            Content->GenerateHLSL(outResult);
        }
        outResult += " )";
    }

    void ASTNodeIdentifier::GenerateHLSL(String& outResult)
    {
        outResult += Identifier.ToString();
    }

    void ASTNodeInteger::GenerateHLSL(String& outResult)
    {
        outResult += std::to_string(IntValue);
    }

    void ASTNodeType::GenerateHLSL(String& outResult)
    {
        outResult += get_type_name(ParamType);
    }

    #pragma endregion

    #pragma region PRINT_AST

    void ASTNodeFunction::PrintAST(int indent)
    {
        print_blank(indent);
        printf("Function:\n");
        Signature->PrintAST(indent + 1);
        Body->PrintAST(indent + 1);
    }

    void ASTNodeFuncSignature::PrintAST(int indent)
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

    void ASTNodeParamList::PrintAST(int indent)
    {
        print_blank(indent);
        printf("ParamList(Count %d):\n", ParamCount);
        if (ParamsHead != nullptr)
        {
            ParamsHead->PrintAST(indent + 1);
        }
    }

    void ASTNodeParam::PrintAST(int indent)
    {
        print_blank(indent);
        printf("Param { ");
        ParamType->PrintAST(0);
        printf("; Name : %s }\n", ParamName.c_str());
        if (NextParam != nullptr) {
            NextParam->PrintAST(indent);
        }
    }

    void ASTNodeStatementList::PrintAST(int indent)
    {
        print_blank(indent);
        printf("StatementList:\n");
        StatementsHead->PrintAST(indent + 1);
    }

    void ASTNodeStatement::PrintAST(int indent)
    {
        TAssertf(StatContent, "StatContent is null");
        StatContent->PrintAST(indent);
        if (NextStatement != nullptr)
        {
            NextStatement->PrintAST(indent);
        }
    }

    void ASTNodeVarDeclaration::PrintAST(int indent)
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
    }

    void ASTNodeAssignment::PrintAST(int indent)
    {
        print_blank(indent);
        printf("Assignment: {\n");
        print_blank(indent + 1);
        printf("%s = \n", LhsVar.c_str());
        TAssertf(RhsExpression != nullptr, "Assignment rhs is null");
        RhsExpression->PrintAST(indent + 1);
        print_blank(indent);
        printf("}\n");
    }

    void ASTNodeReturn::PrintAST(int indent)
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
    }

    void ASTNodeBinaryOperation::PrintAST(int indent)
    {
        TAssertf(left != nullptr && right != nullptr, "Binary operation left or right is null");
        print_blank(indent);
        printf("BinaryOp: ");
        switch (op) {
        case EBinaryOp::OP_ADD: printf("+\n"); break;
        case EBinaryOp::OP_SUB: printf("-\n"); break;
        case EBinaryOp::OP_MUL: printf("*\n"); break;
        case EBinaryOp::OP_DIV: printf("/\n"); break;
        case EBinaryOp::OP_UNDEFINED: break;
        }
        left->PrintAST(indent + 1);
        right->PrintAST(indent + 1);
    }

    void ASTNodePriority::PrintAST(int indent)
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

    void ASTNodeIdentifier::PrintAST(int indent)
    {
        print_blank(indent);
        printf("IdentifierName: %s\n", Identifier.c_str());
    }

    void ASTNodeInteger::PrintAST(int indent)
    {
        print_blank(indent);
        printf("Integer: %d\n", IntValue);
    }

    void ASTNodeType::PrintAST(int indent)
    {
        print_blank(indent);
        printf("Type: %s", get_type_name(ParamType));
    }

#pragma endregion

#pragma region CREATE_AST_NODE

    ASTNode* create_function_node(ASTNode* signature, ASTNode* body)
    {
        TAssertf(signature != nullptr && signature->Type == ENodeType::NODE_FUNC_SIGNATURE, "Signature node type is not correct");
        TAssertf(body != nullptr && body->Type == ENodeType::NODE_STATEMENT_LIST, "Body node type is not correct");

        const auto node = new ASTNodeFunction;
        node->Signature = static_cast<ASTNodeFuncSignature*>(signature);
        node->Body = static_cast<ASTNodeStatementList*>(body);
        return node;
    }

    ASTNode* create_func_signature_node(ASTNode* returnTypeNode, const char *name, ASTNode* params)
    {
        TAssertf(returnTypeNode != nullptr && returnTypeNode->Type == ENodeType::NODE_TYPE, "Return type node type is not correct");
        TAssertf(params != nullptr && params->Type == ENodeType::NODE_PARAM_LIST, "Params node type is not correct");

        const auto node = new ASTNodeFuncSignature;
        node->ReturnType = static_cast<ASTNodeType*>(returnTypeNode);
        node->FuncName = name;
        node->Params = static_cast<ASTNodeParamList*>(params);
        return node;
    }

    ASTNode* create_param_list_node()
    {
        const auto node = new ASTNodeParamList;
        node->ParamCount = 0;
        node->ParamsHead = nullptr;
        node->ParamsTail = nullptr;
        return node;
    }

    void add_param_to_list(ASTNode* list, ASTNode* param)
    {
        TAssertf(list != nullptr && list->Type == ENodeType::NODE_PARAM_LIST, "List node type is not correct");
        TAssertf(param != nullptr && param->Type == ENodeType::NODE_PARAM, "Param node type is not correct");

        if (const auto paramList = static_cast<ASTNodeParamList*>(list))
        {
            if (paramList->ParamCount == 0)
            {
                paramList->ParamsHead = static_cast<ASTNodeParam*>(param);
                paramList->ParamsTail = static_cast<ASTNodeParam*>(param);
            } else {
                paramList->ParamsTail->NextParam = static_cast<ASTNodeParam*>(param);
                paramList->ParamsTail = static_cast<ASTNodeParam*>(param);
            }
            paramList->ParamCount++;
        }
    }

    ASTNode* create_param_node(ASTNode* typeNode, const char *name)
    {
        TAssertf(typeNode != nullptr && typeNode->Type == ENodeType::NODE_TYPE, "Type node type is not correct");

        const auto node = new ASTNodeParam;
        node->ParamType = static_cast<ASTNodeType*>(typeNode);
        node->ParamName = name;
        node->NextParam = nullptr;
        return node;
    }

    ASTNode* create_statement_list_node()
    {
        const auto node = new ASTNodeStatementList;
        node->StatementsHead = nullptr;
        node->StatementsTail = nullptr;
        return node;
    }

    void add_statement_to_list(ASTNode* list, ASTNode* stmt)
    {
        TAssertf(list != nullptr && list->Type == ENodeType::NODE_STATEMENT_LIST, "List node type is not correct");
        TAssertf(stmt != nullptr && stmt->Type == ENodeType::NODE_STATEMENT, "Statement node type is not correct");

        if (const auto stmtList = static_cast<ASTNodeStatementList*>(list); const auto newStmt = static_cast<ASTNodeStatement*>(stmt))
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

    ASTNode* create_statement_node(ASTNode* statContent, EStatType type)
    {
        const auto node = new ASTNodeStatement(EStatType::Stat_UNDEFINED);
        node->StatNodeType = type;
        node->StatContent = statContent;
        node->NextStatement = nullptr;
        return node;
    }

    ASTNode* create_var_decl_node(ASTNode* typeNode, const char *name, ASTNode* init_expr)
    {
        TAssertf(typeNode != nullptr && typeNode->Type == ENodeType::NODE_TYPE, "Type node type is not correct");

        const auto node = new ASTNodeVarDeclaration;
        node->VarDelType = static_cast<ASTNodeType*>(typeNode);
        node->VarName = name;
        if (init_expr != nullptr)
        {
            TAssertf(init_expr->Type == ENodeType::NODE_EXPRESSION, "Init expression node type is not correct");
            node->DelExpression = static_cast<ASTNodeExpression*>(init_expr);
        }
        return node;
    }

    ASTNode* create_assignment_node(const char *lhs, ASTNode* rhs)
    {
        TAssertf(rhs != nullptr && rhs->Type == ENodeType::NODE_EXPRESSION, "Assignment rhs is null");

        const auto node = new ASTNodeAssignment;
        node->LhsVar = lhs;
        node->RhsExpression = static_cast<ASTNodeExpression*>(rhs);
        return node;
    }

    ASTNode* create_return_node(ASTNode* expr)
    {
        TAssertf(expr != nullptr && expr->Type == ENodeType::NODE_EXPRESSION, "Return expression is null");
        const auto node = new ASTNodeReturn;
        node->RetValue = static_cast<ASTNodeExpression*>(expr);
        return node;
    }

    ASTNode* create_binary_op_node(EBinaryOp op, ASTNode* left, ASTNode* right)
    {
        TAssertf(left != nullptr && left->Type == ENodeType::NODE_EXPRESSION, "Binary operation left is null");
        TAssertf(right != nullptr && right->Type == ENodeType::NODE_EXPRESSION, "Binary operation right is null");

        const auto node = new ASTNodeBinaryOperation;
        node->op = op;
        node->left = static_cast<ASTNodeExpression*>(left);
        node->right = static_cast<ASTNodeExpression*>(right);
        return node;
    }

    ASTNode* create_priority_node(ASTNode* expr)
    {
        const auto node = new ASTNodePriority;
        if (expr)
        {
            TAssert(expr->Type == ENodeType::NODE_EXPRESSION);
            node->Content = static_cast<ASTNodeExpression*>(expr);
        }
        return node;
    }

    ASTNode* create_type_node(EVarType type) {
        const auto node = new ASTNodeType;
        node->ParamType = type; // 复用param_type字段
        return node;
    }

    ASTNode* create_identifier_node(const char *name) {
        const auto node = new ASTNodeIdentifier;
        node->Identifier = name;
        return node;
    }

    ASTNode* create_int_literal_node(int value) {
        const auto node = new ASTNodeInteger;
        node->IntValue = value;
        return node;
    }

#pragma endregion

    void post_process_ast(ASTNode* nodeRoot)
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