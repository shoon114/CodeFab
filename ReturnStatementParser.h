#pragma once
#include "IStatementParser.h"

class ReturnStatementParser : public IStatementParser {
public:
	std::unique_ptr<SyntaxNode> Parse(const TokenList& tokenList, size_t& pos) override;
};
