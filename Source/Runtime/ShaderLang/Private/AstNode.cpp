#pragma optimize("", off)
#include "AstNode.h"
#include "Assertion.h"

#include <cstdio>
#include <cstdlib>

namespace Thunder
{

static void print_blank(int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
}

static const char* get_type_name(VarType type) {
    switch (type)
    {
        case TP_INT: return "int";
        case TP_FLOAT: return "float";
        case TP_VOID: return "void";
        default: return "unknown";
    }
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
    const String returnType = get_type_name(ReturnType);
    outResult += returnType + " " + FuncName + "(" + paramList + ");\n";
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
    const String typeName = get_type_name(ParamType);
    outResult += typeName + " " + ParamName;
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
    const String typeName = get_type_name(VarDelType);
    outResult += typeName + " " + VarName;
    if (DelExpression != nullptr)
    {
        outResult += " = ";
        DelExpression->GenerateHLSL(outResult);
    }
    outResult += ";\n";
}

void ASTNodeAssignment::GenerateHLSL(String& outResult)
{
    outResult += String(lhs) + " = ";
    TAssert(rhs != nullptr, "Assignment rhs is null");
    rhs->GenerateHLSL(outResult);
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
    TAssert(left != nullptr && right != nullptr, "Binary operation left or right is null");
    outResult += "(";
    left->GenerateHLSL(outResult);
    switch (op) {
    case OP_ADD: outResult += " + "; break;
    case OP_SUB: outResult += " - "; break;
    case OP_MUL: outResult += " * "; break;
    case OP_DIV: outResult += " / "; break;
    }
    right->GenerateHLSL(outResult);
    outResult += ")";
}

void ASTNodeIdentifier::GenerateHLSL(String& outResult)
{
    outResult += Identifier;
}

void ASTNodeInteger::GenerateHLSL(String& outResult)
{
    outResult += std::to_string(IntValue);
}

#pragma endregion

#pragma region PRINT_AST

void ASTNode::PrintAST(int indent)
{
    print_blank(indent);
}

void ASTNodeFunction::PrintAST(int indent)
{
    ASTNode::PrintAST(indent);
    printf("Function:\n");
    Signature->PrintAST(indent + 1);
    Body->PrintAST(indent + 1);
}

void ASTNodeFuncSignature::PrintAST(int indent)
{
    ASTNode::PrintAST(indent);
    printf("Signature:\n");
    print_blank(indent + 1);
    printf("FuncName: %s\n", FuncName);
    print_blank(indent + 1);
    printf("ReturnType: %s\n", get_type_name(ReturnType));
    Params->PrintAST(indent + 1);
}

void ASTNodeParamList::PrintAST(int indent)
{
    ASTNode::PrintAST(indent);
    printf("ParamList(Count %d):\n", ParamCount);
    ParamsHead->PrintAST(indent + 1);
}

void ASTNodeParam::PrintAST(int indent)
{
    ASTNode::PrintAST(indent);
    printf("Param { Type : %s; Name : %s }\n", get_type_name(ParamType), ParamName);
    if (NextParam != nullptr) {
        NextParam->PrintAST(indent);
    }
}

void ASTNodeStatementList::PrintAST(int indent)
{
    ASTNode::PrintAST(indent);
    printf("StatementList:\n");
    StatementsHead->PrintAST(indent + 1);
}

void ASTNodeStatement::PrintAST(int indent)
{
    TAssert(StatContent, "StatContent is null");
    ASTNode::PrintAST(indent);
    StatContent->PrintAST(indent);
    if (NextStatement != nullptr)
    {
        NextStatement->PrintAST(indent);
    }
}

void ASTNodeVarDeclaration::PrintAST(int indent)
{
    printf("VarDecl: {\n");
    print_blank(indent + 1);
    printf("Type : %s; Name : %s\n", get_type_name(VarDelType), VarName);
    if (DelExpression != nullptr)
    {
        DelExpression->PrintAST(indent + 1);
    }
    print_blank(indent);
    printf("}\n");
}

void ASTNodeAssignment::PrintAST(int indent)
{
    printf("Assignment: {\n");
    print_blank(indent + 1);
    printf("%s = \n", lhs);
    TAssert(rhs != nullptr, "Assignment rhs is null");
    rhs->PrintAST(indent + 1);
    print_blank(indent);
    printf("}\n");
}

void ASTNodeReturn::PrintAST(int indent)
{
    ASTNode::PrintAST(indent);
    //todo
}

void ASTNodeBinaryOperation::PrintAST(int indent)
{
    TAssert(left != nullptr && right != nullptr, "Binary operation left or right is null");
    ASTNode::PrintAST(indent);
    printf("BinaryOp: ");
    switch (op) {
    case OP_ADD: printf("+\n"); break;
    case OP_SUB: printf("-\n"); break;
    case OP_MUL: printf("*\n"); break;
    case OP_DIV: printf("/\n"); break;
    }
    left->PrintAST(indent + 1);
    right->PrintAST(indent + 1);
}

void ASTNodeIdentifier::PrintAST(int indent)
{
    ASTNode::PrintAST(indent);
    printf("Identifier: %s\n", Identifier);
}

void ASTNodeInteger::PrintAST(int indent)
{
    ASTNode::PrintAST(indent);
    printf("Integer: %d\n", IntValue);
}

#pragma endregion

#pragma region CREATE_AST_NODE

ASTNode* create_function_node(ASTNode* signature, ASTNode* body)
{
    TAssert(signature != nullptr && signature->type == NODE_FUNC_SIGNATURE, "Signature node type is not correct");
    TAssert(body != nullptr && body->type == NODE_STATEMENT_LIST, "Body node type is not correct");

    const auto node = new ASTNodeFunction;
    node->Signature = static_cast<ASTNodeFuncSignature*>(signature);
    node->Body = static_cast<ASTNodeStatementList*>(body);
    return node;
}

ASTNode* create_func_signature_node(const ASTNode* returnTypeNode, const char *name, ASTNode* params)
{
    TAssert(returnTypeNode != nullptr && returnTypeNode->type == NODE_TYPE, "Return type node type is not correct");
    TAssert(params != nullptr && params->type == NODE_PARAM_LIST, "Params node type is not correct");

    const auto node = new ASTNodeFuncSignature;
    node->ReturnType = static_cast<const ASTNodeType*>(returnTypeNode)->ParamType;
    node->FuncName = strdup(name);
    node->Params = params;
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
    TAssert(list != nullptr && list->type == NODE_PARAM_LIST, "List node type is not correct");
    TAssert(param != nullptr && param->type == NODE_PARAM, "Param node type is not correct");

    if(const auto paramList = static_cast<ASTNodeParamList*>(list))
    {
        if(paramList->ParamCount == 0)
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

ASTNode* create_param_node(const ASTNode* typeNode, const char *name)
{
    TAssert(typeNode != nullptr && typeNode->type == NODE_TYPE, "Type node type is not correct");

    const auto node = new ASTNodeParam;
    node->ParamType = static_cast<const ASTNodeType*>(typeNode)->ParamType;
    node->ParamName = strdup(name);
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
    TAssert(list != nullptr && list->type == NODE_STATEMENT_LIST, "List node type is not correct");
    TAssert(stmt != nullptr && stmt->type == NODE_STATEMENT, "Statement node type is not correct");

    if(const auto stmtList = static_cast<ASTNodeStatementList*>(list); const auto newStmt = static_cast<ASTNodeStatement*>(stmt))
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

ASTNode* create_statement_node(ASTNode* statContent, StatType type)
{
    const auto node = new ASTNodeStatement;
    node->StatNodeType = type;
    node->StatContent = statContent;
    node->NextStatement = nullptr;
    return node;
}

ASTNode* create_var_decl_node(const ASTNode* typeNode, char *name, ASTNode* init_expr) {
    TAssert(typeNode != nullptr && typeNode->type == NODE_TYPE, "Type node type is not correct");

    const auto node = new ASTNodeVarDeclaration;
    node->VarDelType = static_cast<const ASTNodeType*>(typeNode)->ParamType;
    node->VarName = strdup(name);
    node->DelExpression = init_expr;
    return node;
}

ASTNode* create_assignment_node(char *lhs, ASTNode* rhs) {
    const auto node = new ASTNodeAssignment;
    node->lhs = strdup(lhs);
    node->rhs = rhs;
    return node;
}

ASTNode* create_return_node(ASTNode* expr)
{
    TAssert(expr != nullptr && expr->type == NODE_EXPRESSION, "Return expression is null");
    const auto node = new ASTNodeReturn;
    node->RetValue = static_cast<ASTNodeExpression*>(expr);
    return node;
}

ASTNode* create_expression_node() {
    const auto node = new ASTNodeExpression;
    return node;
}

ASTNode* create_binary_op_node(BinaryOp op, ASTNode* left, ASTNode* right) {
    const auto node = new ASTNodeBinaryOperation;
    node->op = op;
    node->left = left;
    node->right = right;
    return node;
}

ASTNode* create_type_node(VarType type) {
    const auto node = new ASTNodeType;
    node->ParamType = type; // 复用param_type字段
    return node;
}

ASTNode* create_identifier_node(char *name) {
    const auto node = new ASTNodeIdentifier;
    node->Identifier = strdup(name);
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
    printf("generate hlsl\n%s", outHlsl.c_str());
}
    
}
#pragma optimize("", on)