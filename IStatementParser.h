#pragma once
#include <memory>
#include "Token.h"
#include "SyntaxNode.h"

class IStatementParser {
public:
	virtual ~IStatementParser() = default;
	virtual std::unique_ptr<SyntaxNode> Parse(const TokenList& tokenList, size_t& pos) = 0;
};
