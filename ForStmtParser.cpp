#include "ForStmtParser.h"
#include "StatementParserRegistry.h"

namespace {
StatementParserRegistrar<ForStmtParser> registrar(TokenType::KwFor);
}

std::unique_ptr<SyntaxNode> ForStmtParser::Parse(const TokenList& tokenList, size_t& pos) {
	return nullptr;
}
