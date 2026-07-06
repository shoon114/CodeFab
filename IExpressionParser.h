#pragma once
#include <memory>
#include "Token.h"
#include "SyntaxNode.h"

class IExpressionParser {
public:
	virtual ~IExpressionParser() = default;
	virtual std::unique_ptr<SyntaxNode> Parse(const TokenList& tokenList, size_t& pos) = 0;
};
