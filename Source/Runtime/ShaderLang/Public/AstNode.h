#pragma once
#include "Container.h"
#include "NameHandle.h"

namespace Thunder
{

    enum class EBasalNodeType : uint8
    {
        NODE_FUNCTION,
        NODE_FUNC_SIGNATURE,
        NODE_PARAM_LIST,
        NODE_PARAM,
        NODE_STATEMENT_LIST,
        NODE_STATEMENT,
        NODE_EXPRESSION,
        NODE_TYPE
    };

    enum class EStatementType : uint8
    {
        Stat_DECLARE,
        Stat_ASSIGN,
        Stat_RETURN,
        Stat_UNDEFINED
    };

    enum class EExpressionType : uint8
    {
        EXPR_BINARY_OP,
        EXPR_PRIORITY,
        EXPR_IDENTIFIER,
        EXPR_INTEGER,
        EXPR_UNDEFINED
    };

    enum class EVarType : uint8
    {
        TP_INT,
        TP_FLOAT,
        TP_VOID,
        TP_UNDEFINED
    };

    enum class EBinaryOp : uint8
    {
        OP_ADD,
        OP_SUB,
        OP_MUL,
        OP_DIV,
        OP_UNDEFINED
    };

    class ASTNode
    {
    public:
        ASTNode(EBasalNodeType type) : Type(type) {}
        virtual ~ASTNode() = default;

        virtual void GenerateHLSL(String& outResult) = 0;
        virtual void GenerateDXIL();
        virtual void PrintAST(int indent) = 0;

    public:
        EBasalNodeType Type;
    };

    class ASTNodeFunction : public ASTNode
    {
    public:
        ASTNodeFunction() : ASTNode(EBasalNodeType::NODE_FUNCTION) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        class ASTNodeFuncSignature* Signature = nullptr;
        class ASTNodeStatementList* Body = nullptr;
    };

    class ASTNodeFuncSignature : public ASTNode
    {
    public:
        ASTNodeFuncSignature() : ASTNode(EBasalNodeType::NODE_FUNC_SIGNATURE) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        class ASTNodeType* ReturnType = nullptr;
        NameHandle FuncName = nullptr;
        class ASTNodeParamList* Params = nullptr;
    };

    class ASTNodeParamList : public ASTNode
    {
    public:
        ASTNodeParamList() : ASTNode(EBasalNodeType::NODE_PARAM_LIST) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        int ParamCount = 0;
        class ASTNodeParam* ParamsHead = nullptr;
        ASTNodeParam* ParamsTail = nullptr;
    };

    class ASTNodeParam : public ASTNode
    {
    public:
        ASTNodeParam() : ASTNode(EBasalNodeType::NODE_PARAM) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        ASTNodeType* ParamType = nullptr;
        NameHandle ParamName = nullptr;
        ASTNodeParam*  NextParam = nullptr;
    };

    class ASTNodeStatementList : public ASTNode
    {
    public:
        ASTNodeStatementList() : ASTNode(EBasalNodeType::NODE_STATEMENT_LIST) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        class ASTNodeStatement* StatementsHead = nullptr;
        ASTNodeStatement* StatementsTail = nullptr;
    };

    class ASTNodeStatement : public ASTNode
    {
    public:
        ASTNodeStatement(EStatementType statType)
        : ASTNode(EBasalNodeType::NODE_STATEMENT), StatNodeType(statType) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        EStatementType StatNodeType;
        ASTNodeStatement* NextStatement = nullptr;
    };

    class ASTNodeVarDeclaration : public ASTNodeStatement
    {
    public:
        ASTNodeVarDeclaration() : ASTNodeStatement(EStatementType::Stat_DECLARE) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        ASTNodeType* VarDelType = nullptr;
        NameHandle VarName = nullptr;
        class ASTNodeExpression* DelExpression = nullptr;
    };

    class ASTNodeAssignment : public ASTNodeStatement
    {
    public:
        ASTNodeAssignment() : ASTNodeStatement(EStatementType::Stat_ASSIGN) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        NameHandle LhsVar = nullptr;
        ASTNodeExpression* RhsExpression = nullptr;
    };

    class ASTNodeReturn : public ASTNodeStatement
    {
    public:
        ASTNodeReturn() : ASTNodeStatement(EStatementType::Stat_RETURN) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        ASTNodeExpression* RetValue = nullptr;
    };

    class ASTNodeExpression : public ASTNode
    {
    public:
        ASTNodeExpression(EExpressionType exprType)
            : ASTNode(EBasalNodeType::NODE_EXPRESSION), ExprType(exprType) {}
    public:
        EExpressionType ExprType;
    };

    class ASTNodeBinaryOperation : public ASTNodeExpression
    {
    public:
        ASTNodeBinaryOperation() : ASTNodeExpression (EExpressionType::EXPR_BINARY_OP) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        EBinaryOp op = EBinaryOp::OP_UNDEFINED;
        ASTNodeExpression* left = nullptr;
        ASTNodeExpression* right = nullptr;
    };

    class ASTNodePriority : public ASTNodeExpression
    {
    public:
        ASTNodePriority() : ASTNodeExpression(EExpressionType::EXPR_PRIORITY) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        ASTNodeExpression * Content = nullptr;
    };

    class ASTNodeIdentifier : public ASTNodeExpression
    {
    public:
        ASTNodeIdentifier() : ASTNodeExpression(EExpressionType::EXPR_IDENTIFIER) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        NameHandle Identifier = nullptr;
    };

    class ASTNodeInteger : public ASTNodeExpression
    {
    public:
        ASTNodeInteger() : ASTNodeExpression(EExpressionType::EXPR_INTEGER) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        int IntValue = 0;
    };

    class ASTNodeType : public ASTNode
    {
    public:
        ASTNodeType() : ASTNode(EBasalNodeType::NODE_TYPE) {}

        void GenerateHLSL(String& outResult) override;
        void PrintAST(int indent) override;
    public:
        EVarType ParamType = EVarType::TP_UNDEFINED;
    };

    ASTNode* create_function_node(ASTNode* signature, ASTNode* body);
    ASTNode* create_func_signature_node(ASTNode* returnTypeNode, const char* name, ASTNode* params);
    ASTNode* create_param_list_node();
    void add_param_to_list(ASTNode* list, ASTNode* param);
    ASTNode* create_param_node(ASTNode* typeNode, const char* name);
    ASTNode* create_statement_list_node();
    void add_statement_to_list(ASTNode* list, ASTNode* stmt);
    ASTNode* create_var_decl_node(ASTNode* typeNode, const char* name, ASTNode* init_expr);
    ASTNode* create_assignment_node(const char* lhs, ASTNode* rhs);
    ASTNode* create_return_node(ASTNode* expr);
    ASTNode* create_binary_op_node(EBinaryOp op, ASTNode* left, ASTNode* right);
    ASTNode* create_priority_node(ASTNode* expr);
    ASTNode* create_identifier_node(const char* name);
    ASTNode* create_int_literal_node(int value);
    ASTNode* create_type_node(EVarType type);
    void post_process_ast(ASTNode* nodeRoot);

}
