#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 符号表：存储变量名和值
#define MAX_VARS 100
typedef struct {
    char *name;
    int value;
} Variable;

void set_variable_value(const char *name, int value);
 
int get_variable_value(const char *name);

// 定义抽象语法树节点
typedef enum {
    NODE_NUMBER,
    NODE_IDENTIFIER,
    NODE_ADD,
    NODE_SUB,
    NODE_MUL,
    NODE_DIV,
    NODE_ASSIGN,
    NODE_PAREN
} NodeType;
 
typedef struct ASTNode {
    NodeType type;
    int value; // 用于存储数字
    char *name; // 用于存储变量名
    struct ASTNode *left;
    struct ASTNode *right;
} ASTNode;
 
ASTNode* createNode(NodeType type, int value, char *name, ASTNode *left, ASTNode *right);
 
int evaluateNode(ASTNode *node);
 
void SHADERLANG_API printAST(ASTNode *node, int level);
 
void SHADERLANG_API printSymbolTable();