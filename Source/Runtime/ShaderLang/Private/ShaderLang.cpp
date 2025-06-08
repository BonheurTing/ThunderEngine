#include "ShaderLang.h"
#include "AstNode.h"

namespace Thunder
{

	void ParserDestroy(shader_lang_state* state)
	{
		if (!state) return;
    
		// 释放AST
		AstFree(state->AstRoot);

		free(state);
	}

	int ParserParse(shader_lang_state* state)
	{
		// 重置状态
		if (state->AstRoot) {
			AstFree(state->AstRoot);
			state->AstRoot = nullptr;
		}
    
		// 开始解析
		//yyparse(state);
		return 1;
	}

	ast_node* AstCreateNode(EBasalNodeType type, int lineNo)
	{
		return nullptr;
	}

	void AstAddChild(ast_node* parent, ast_node* child)
	{
	}

	void AstFree(ast_node* node)
	{
	}

	void DebugLog(shader_lang_state* state, enum_debug_level debugLevel, const char* message, int lineNo)
	{
		printf("%s", message);
	}
}
