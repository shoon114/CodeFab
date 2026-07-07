#include "BlockParser.h"
#include "StatementParserRegistry.h"

namespace {
StatementParserRegistrar<BlockParser> registrar(TokenType::LBrace);
}

std::unique_ptr<SyntaxNode> BlockParser::Parse(const TokenList& tokenList, size_t& pos) {
	// TODO: implement via TDD.
	return nullptr;
}
