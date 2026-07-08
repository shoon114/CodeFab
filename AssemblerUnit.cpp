#include "AssemblerUnit.h"
#include "StatementParserRegistry.h"
#include <stdexcept>

std::unique_ptr<SyntaxNode> AssemblerUnit::Parse(const TokenList& tokenList) {
	auto programNode = std::make_unique<SyntaxNode>();
	programNode->type = NodeType::Program;

	size_t pos = 0;
	while (pos < tokenList.size() && tokenList[pos].type != TokenType::EndOfFile) {
		std::shared_ptr<IStatementParser> parser = StatementParserRegistry::Instance().Resolve(tokenList[pos].type);
		if (parser == nullptr) {
			throw std::runtime_error("Unexpected token at line " + std::to_string(tokenList[pos].line));
		}
		programNode->children.push_back(parser->Parse(tokenList, pos));
	}

	return programNode;
}
