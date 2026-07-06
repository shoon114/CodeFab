#include "AssemblerUnit.h"
#include "VarDeclareParser.h"

AssemblerUnit::AssemblerUnit(std::shared_ptr<IExpressionParser> exprParser) : exprParser(std::move(exprParser)) {
	statementRegistry.Register(TokenType::KwVar, std::make_shared<VarDeclareParser>(*this->exprParser));
}

std::unique_ptr<SyntaxNode> AssemblerUnit::Parse(const TokenList& tokenList) {
	if (tokenList.empty()) {
		return nullptr;
	}

	size_t pos = 0;
	IStatementParser* parser = statementRegistry.Resolve(tokenList[pos].type);
	if (parser == nullptr) {
		return nullptr;
	}

	return parser->Parse(tokenList, pos);
}
