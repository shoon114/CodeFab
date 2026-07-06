#pragma once
#include <memory>
#include "Token.h"
#include "SyntaxNode.h"
#include "IExpressionParser.h"

class AssemblerUnit {
public:
	explicit AssemblerUnit(std::shared_ptr<IExpressionParser> exprParser);
	std::unique_ptr<SyntaxNode> Parse(const TokenList& tokenList);

private:
	std::shared_ptr<IExpressionParser> exprParser;
};
