#pragma once
#pragma once
#include "IStatementParser.h"
#include "IExpressionParser.h"
class PrintStatementParser : public IStatementParser {
public:
	explicit PrintStatementParser(IExpressionParser& exprParser) : exprParser(exprParser) {}

	std::unique_ptr<SyntaxNode> Parse(const TokenList& tokenList, size_t& pos) override;

private:
	IExpressionParser& exprParser;
};
