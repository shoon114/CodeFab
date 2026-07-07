#pragma once
#include "IStatementParser.h"
#include "IExpressionParser.h"

class IfStatementParser : public IStatementParser {
public:
	explicit IfStatementParser(IExpressionParser& exprParser) : exprParser(exprParser) {}

	std::unique_ptr<SyntaxNode> Parse(const TokenList& tokenList, size_t& pos) override;

private:
	IExpressionParser& exprParser;
};
