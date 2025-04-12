
#include "AstNode.h"

Variable symbol_table[MAX_VARS];
int symbol_count = 0;

void set_variable_value(const char *name, int value) {
	// 检查符号表中是否已有该变量
	for (int i = 0; i < symbol_count; i++) {
		if (strcmp(symbol_table[i].name, name) == 0) {
			symbol_table[i].value = value;
			return;
		}
	}
	// 如果没有，插入新变量
	symbol_table[symbol_count].name = strdup(name);
	symbol_table[symbol_count].value = value;
	symbol_count++;
}

int get_variable_value(const char *name) {
	// 查找变量值
	for (int i = 0; i < symbol_count; i++) {
		if (strcmp(symbol_table[i].name, name) == 0) {
			return symbol_table[i].value;
		}
	}
	// 如果未找到变量，报错并退出
	printf("Error: Undefined variable %s\n", name);
	exit(1);
}

ASTNode* createNode(NodeType type, int value, char *name, ASTNode *left, ASTNode *right) {
	ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));
	node->type = type;
	node->value = value;
	node->name = name;
	node->left = left;
	node->right = right;
	return node;
}

int evaluateNode(ASTNode *node) {
	if (node == NULL) return 0;
	switch (node->type) {
	case NODE_NUMBER:
		return node->value;
	case NODE_IDENTIFIER:
		return get_variable_value(node->name);
	case NODE_ADD:
		return evaluateNode(node->left) + evaluateNode(node->right);
	case NODE_SUB:
		return evaluateNode(node->left) - evaluateNode(node->right);
	case NODE_MUL:
		return evaluateNode(node->left) * evaluateNode(node->right);
	case NODE_DIV:
		return evaluateNode(node->left) / evaluateNode(node->right);
	case NODE_ASSIGN:
		return node->right->value;
	case NODE_PAREN:
		return evaluateNode(node->left);
	default:
		return 0;
	}
}

void printAST(ASTNode *node, int level) {
	if (node == NULL) return;
	for (int i = 0; i < level; i++) printf("  ");
	switch (node->type) {
	case NODE_NUMBER:
		printf("Number: %d\n", node->value);
		break;
	case NODE_IDENTIFIER:
		printf("Identifier: %s\n", node->name);
		break;
	case NODE_ADD:
		printf("ADD\n");
		break;
	case NODE_SUB:
		printf("SUB\n");
		break;
	case NODE_MUL:
		printf("MUL\n");
		break;
	case NODE_DIV:
		printf("DIV\n");
		break;
	case NODE_ASSIGN:
		printf("Assign\n");
		break;
	case NODE_PAREN:
		printf("Paren\n");
		break;
	}
	printAST(node->left, level + 1);
	printAST(node->right, level + 1);
}

void printSymbolTable() {
	printf("Symbol Table:\n");
	for (int i = 0; i < symbol_count; i++) {
		printf("%s = %d\n", symbol_table[i].name, symbol_table[i].value);
	}
}