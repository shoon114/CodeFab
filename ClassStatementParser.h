#pragma once
#include "IStatementParser.h"

class ClassStatementParser : public IStatementParser {
public:
	std::unique_ptr<SyntaxNode> Parse(const TokenList& tokenList, size_t& pos) override;

private:
	std::unique_ptr<SyntaxNode> ParseMethod(const TokenList& tokenList, size_t& pos);
};
