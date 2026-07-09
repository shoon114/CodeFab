#include "ImportStatementParser.h"
#include "StatementParserRegistry.h"
#include <stdexcept>

namespace {
StatementParserRegistrar<ImportStatementParser> registrar(TokenType::KwImport);
}

std::unique_ptr<SyntaxNode> ImportStatementParser::Parse(const TokenList& tokenList, size_t& pos) {
	throw std::runtime_error("not implemented");
}
