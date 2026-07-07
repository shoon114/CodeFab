#include "StatementParserRegistry.h"

StatementParserRegistry& StatementParserRegistry::Instance() {
	static StatementParserRegistry instance;
	return instance;
}

void StatementParserRegistry::Register(TokenType leadingToken, StatementParserFactory factory) {
	factories[leadingToken] = std::move(factory);
}

std::shared_ptr<IStatementParser> StatementParserRegistry::Resolve(TokenType leadingToken, IExpressionParser& exprParser) const {
	auto it = factories.find(leadingToken);
	if (it == factories.end()) {
		return nullptr;
	}
	return it->second(exprParser);
}
