#pragma once
#include "IStatementParser.h"
#include "IExpressionParser.h"

class VarDeclareParser : public IStatementParser {
public:
	explicit VarDeclareParser(IExpressionParser& exprParser) : exprParser(exprParser) {}

	std::unique_ptr<SyntaxNode> Parse(const TokenList& tokenList, size_t& pos) override;

private:
	IExpressionParser& exprParser;
};
