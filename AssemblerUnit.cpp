#include "AssemblerUnit.h"
#include "StatementParserRegistry.h"

AssemblerUnit::AssemblerUnit(std::shared_ptr<IExpressionParser> exprParser) : exprParser(std::move(exprParser)) {
}

std::unique_ptr<SyntaxNode> AssemblerUnit::Parse(const TokenList& tokenList) {
	if (tokenList.empty()) {
		return nullptr;
	}

	size_t pos = 0;
	std::shared_ptr<IStatementParser> parser = StatementParserRegistry::Instance().Resolve(tokenList[pos].type, *exprParser);
	if (parser == nullptr) {
		return nullptr;
	}

	return parser->Parse(tokenList, pos);
}
