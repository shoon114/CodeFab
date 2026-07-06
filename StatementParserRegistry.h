#pragma once
#include <memory>
#include <unordered_map>
#include "Token.h"
#include "IStatementParser.h"

class StatementParserRegistry {
public:
	void Register(TokenType leadingToken, std::shared_ptr<IStatementParser> parser);
	IStatementParser* Resolve(TokenType leadingToken) const;

private:
	std::unordered_map<TokenType, std::shared_ptr<IStatementParser>> parsers;
};
