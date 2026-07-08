#include "AssemblerUnit.h"
#include "StatementParserRegistry.h"
#include <stdexcept>

namespace {
	bool HasMoreStatementsToParse(const TokenList& tokenList, size_t pos) {
		return pos < tokenList.size() && tokenList[pos].type != TokenType::EndOfFile;
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
	}

	return programNode;
}
