#include "StatementParserRegistry.h"

void StatementParserRegistry::Register(TokenType leadingToken, std::shared_ptr<IStatementParser> parser) {
	parsers[leadingToken] = std::move(parser);
}

IStatementParser* StatementParserRegistry::Resolve(TokenType leadingToken) const {
	auto it = parsers.find(leadingToken);
	if (it == parsers.end()) {
		return nullptr;
	}
	return it->second.get();
}
