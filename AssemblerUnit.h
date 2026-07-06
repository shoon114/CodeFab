#pragma once
#include <memory>
#include "Token.h"
#include "SyntaxNode.h"

class AssemblerUnit {
public:
	std::unique_ptr<SyntaxNode> Parse(const TokenList& tokenList);

private:
	std::unique_ptr<SyntaxNode> ParseVarDeclare(const TokenList& tokenList, size_t& pos);
};
