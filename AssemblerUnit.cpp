#include "AssemblerUnit.h"
#include "StatementParserRegistry.h"
#include <stdexcept>

namespace {
	bool HasMoreStatementsToParse(const TokenList& tokenList, size_t pos) {
		return pos < tokenList.size() && tokenList[pos].type != TokenType::EndOfFile;
	}

	// Statement parsers like PrintStatementParser/VarDeclareParser already
	// consume their own trailing ';'. But a bare expression used as a
	// top-level statement (e.g. "a = a + 5;") resolves directly to
	// ExpressionParser, which -- being reused as a sub-expression parser by
	// VarDeclareParser/IfStatementParser/etc -- must NOT consume a trailing
	// ';' itself. Without this, that leftover ';' would be mistaken for the
	// start of the next statement and fail to resolve. Swallow one optional
	// trailing ';' here so both cases work uniformly.
	void ConsumeOptionalTrailingSemicolon(const TokenList& tokenList, size_t& pos) {
		if (pos < tokenList.size() && tokenList[pos].type == TokenType::Semicolon) {
			pos++;
		}
	}
}

std::unique_ptr<SyntaxNode> AssemblerUnit::Parse(const TokenList& tokenList) {
	auto programNode = std::make_unique<ProgramNode>();

	size_t pos = 0;
	while (HasMoreStatementsToParse(tokenList, pos)) {
		std::shared_ptr<IStatementParser> parser = StatementParserRegistry::Instance().Resolve(tokenList[pos].type);
		if (parser == nullptr) {
			throw std::runtime_error("Unexpected token at line " + std::to_string(tokenList[pos].line));
		}
		programNode->children.push_back(parser->Parse(tokenList, pos));
		ConsumeOptionalTrailingSemicolon(tokenList, pos);
	}

	return programNode;
}
