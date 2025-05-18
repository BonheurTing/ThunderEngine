#pragma once
#include <string>

typedef enum {
    NODE_FUNCTION,
    NODE_FUNC_SIGNATURE,
    NODE_PARAM_LIST,
    NODE_PARAM,
    NODE_STATEMENT_LIST,
    NODE_STATEMENT,
    NODE_VAR_DECLARATION,
    NODE_ASSIGNMENT,
    NODE_RETURN,
    NODE_EXPRESSION,
    NODE_TYPE,
    NODE_IDENTIFIER,
    NODE_INTEGER,
    NODE_BINARY_OP
} NodeType;

typedef enum
{
    Stat_DECLARE,
    Stat_ASSIGN,
    Stat_RETURN,
    Stat_UNDEFINED
} StatType;

typedef enum {
    TP_INT,
    TP_FLOAT,
    TP_VOID,
    TP_UNDEFINED
} VarType;

typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV
} BinaryOp;

class ASTNode
{
public:
    ASTNode(NodeType type) : type(type) {}
    virtual ~ASTNode() = default;

    virtual void PrintAST(int indent);
    virtual void GenerateHLSL(std::string& outResult) {}
    virtual void GenerateDXIL();
    
public:
    NodeType type;
};

class ASTNodeFunction : public ASTNode
{
public:
    ASTNodeFunction() : ASTNode(NODE_FUNCTION) {}
    void PrintAST(int indent) override;
public:
    class ASTNodeFuncSignature* Signature = nullptr;
    class ASTNodeStatementList* Body = nullptr;
};

class ASTNodeFuncSignature : public ASTNode
{
public:
    ASTNodeFuncSignature() : ASTNode(NODE_FUNC_SIGNATURE) {}
    void PrintAST(int indent) override;
public:
    VarType ReturnType = TP_UNDEFINED;
    char* FuncName = nullptr;
    ASTNode* Params = nullptr;
};

class ASTNodeParamList : public ASTNode
{
public:
    ASTNodeParamList() : ASTNode(NODE_PARAM_LIST) {}
    void PrintAST(int indent) override;
public:
    int ParamCount = 0;
    class ASTNodeParam* ParamsHead = nullptr;
    ASTNodeParam* ParamsTail = nullptr;
};

class ASTNodeParam : public ASTNode
{
public:
    ASTNodeParam() : ASTNode(NODE_PARAM) {}
    void PrintAST(int indent) override;
public:
    VarType ParamType = TP_UNDEFINED;
    char* ParamName = nullptr;
    ASTNodeParam*  NextParam = nullptr;
};

class ASTNodeStatementList : public ASTNode
{
public:
    ASTNodeStatementList() : ASTNode(NODE_STATEMENT_LIST) {}
    void PrintAST(int indent) override;
public:
    class ASTNodeStatement* StatementsHead = nullptr;
    ASTNodeStatement* StatementsTail = nullptr;
};

class ASTNodeStatement : public ASTNode
{
public:
    ASTNodeStatement() : ASTNode(NODE_STATEMENT) {}
    void PrintAST(int indent) override;
public:
    StatType StatNodeType = Stat_UNDEFINED;
    ASTNode* StatContent = nullptr;
    ASTNodeStatement* NextStatement = nullptr;
};

class ASTNodeVarDeclaration : public ASTNode
{
public:
    ASTNodeVarDeclaration() : ASTNode(NODE_VAR_DECLARATION) {}
    void PrintAST(int indent) override;
public:
    VarType VarDelType = TP_UNDEFINED;
    char* VarName = nullptr;
    ASTNode* DelExpression = nullptr;
};

class ASTNodeAssignment : public ASTNode
{
public:
    ASTNodeAssignment() : ASTNode(NODE_ASSIGNMENT) {}
    void PrintAST(int indent) override;
public:
    char* lhs = nullptr;
    ASTNode* rhs = nullptr;
};

class ASTNodeReturn : public ASTNode
{
public:
    ASTNodeReturn() : ASTNode(NODE_RETURN) {}
    void PrintAST(int indent) override;
public:
    ASTNode* RetValue = nullptr;
};

class ASTNodeExpression : public ASTNode
{
public:
    ASTNodeExpression() : ASTNode(NODE_EXPRESSION) {}
    void PrintAST(int indent) override;
public:
};

class ASTNodeBinaryOperation : public ASTNode
{
public:
    ASTNodeBinaryOperation() : ASTNode(NODE_BINARY_OP) {}
    void PrintAST(int indent) override;
public:
    BinaryOp op = OP_ADD;
    ASTNode* left = nullptr;
    ASTNode* right = nullptr;
};

class ASTNodeType : public ASTNode
{
public:
    ASTNodeType() : ASTNode(NODE_TYPE) {}
public:
    VarType ParamType = TP_UNDEFINED;
};

class ASTNodeIdentifier : public ASTNode
{
public:
    ASTNodeIdentifier() : ASTNode(NODE_IDENTIFIER) {}
    void PrintAST(int indent) override;
public:
    char* Identifier = nullptr;
};

class ASTNodeInteger : public ASTNode
{
public:
    ASTNodeInteger() : ASTNode(NODE_INTEGER) {}
    void PrintAST(int indent) override;
public:
    int IntValue = 0;
};

ASTNode* create_function_node(ASTNode* signature, ASTNode* body);
ASTNode* create_func_signature_node(const ASTNode* returnTypeNode, const char* name, ASTNode* params);
ASTNode* create_param_list_node();
void add_param_to_list(ASTNode* list, ASTNode* param);
ASTNode* create_param_node(const ASTNode* typeNode, const char* name);
ASTNode* create_statement_list_node();
void add_statement_to_list(ASTNode* list, ASTNode* stmt);
ASTNode* create_statement_node(ASTNode* statContent, StatType type);
ASTNode* create_var_decl_node(const ASTNode* typeNode, char* name, ASTNode* init_expr);
ASTNode* create_assignment_node(char* lhs, ASTNode* rhs);
ASTNode* create_return_node(ASTNode* expr);
ASTNode* create_expression_node();
ASTNode* create_binary_op_node(BinaryOp op, ASTNode* left, ASTNode* right);
ASTNode* create_type_node(VarType type);
ASTNode* create_identifier_node(char* name);
ASTNode* create_int_literal_node(int value);
void print_ast(ASTNode* nodeRoot);