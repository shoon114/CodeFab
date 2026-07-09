#include "ImportStatementParser.h"
#include "StatementParserRegistry.h"
#include <stdexcept>
#include <string>

namespace {
	StatementParserRegistrar<ImportStatementParser> registrar(TokenType::KwImport);

	TokenType PeekType(const TokenList& tokenList, size_t pos) {
		return pos < tokenList.size() ? tokenList[pos].type : TokenType::EndOfFile;
	}

	int PeekLine(const TokenList& tokenList, size_t pos) {
		return pos < tokenList.size() ? tokenList[pos].line : tokenList.back().line;
	}
}

std::unique_ptr<SyntaxNode> ImportStatementParser::Parse(const TokenList& tokenList, size_t& pos) {
	if (pos >= tokenList.size()) {
		throw std::runtime_error("Expected import statement");
	}
	const Token& importToken = tokenList[pos++];

	if (PeekType(tokenList, pos) != TokenType::String) {
		throw std::runtime_error("Expected string literal after import at line " + std::to_string(PeekLine(tokenList, pos)));
	}
	Token pathToken = tokenList[pos++];

	if (PeekType(tokenList, pos) != TokenType::KwAlias) {
		throw std::runtime_error("Expected 'alias' after import path at line " + std::to_string(PeekLine(tokenList, pos)));
	}
	pos++;

	if (PeekType(tokenList, pos) != TokenType::Identifier) {
		throw std::runtime_error("Expected alias name after 'alias' at line " + std::to_string(PeekLine(tokenList, pos)));
	}
	Token aliasToken = tokenList[pos++];

	if (PeekType(tokenList, pos) != TokenType::Semicolon) {
		throw std::runtime_error("Expected ';' after import statement at line " + std::to_string(PeekLine(tokenList, pos)));
	}
	pos++;

	auto node = std::make_unique<ImportStmtNode>();
	node->token = importToken;
	node->pathToken = pathToken;
	node->aliasToken = aliasToken;
	return node;
}
