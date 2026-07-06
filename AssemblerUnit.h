#pragma once
#include <memory>
#include "Token.h"
#include "SyntaxNode.h"

class AssemblerUnit {
public:
	std::unique_ptr<SyntaxNode> Parse(const TokenList& tokenList);
};
