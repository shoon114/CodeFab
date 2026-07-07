#pragma once
#include "IStatementParser.h"
#include "IExpressionParser.h"

class BlockParser : public IStatementParser {
public:
	explicit BlockParser(IExpressionParser& exprParser) : exprParser(exprParser) {}

	std::unique_ptr<SyntaxNode> Parse(const TokenList& tokenList, size_t& pos) override;

private:
	IExpressionParser& exprParser;
};
