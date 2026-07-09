#include "ImportStatementParser.h"
#include "StatementParserRegistry.h"
#include <stdexcept>
#include <string>

namespace {
StatementParserRegistrar<ImportStatementParser> registrar(TokenType::KwImport);

int PeekLine(const TokenList& tokenList, size_t pos) {
	return pos < tokenList.size() ? tokenList[pos].line : tokenList.back().line;
}
}

std::unique_ptr<SyntaxNode> ImportStatementParser::Parse(const TokenList& tokenList, size_t& pos) {
	const Token& importToken = tokenList[pos++]; // 'import'

	if (pos >= tokenList.size() || tokenList[pos].type != TokenType::String) {
		throw std::runtime_error("Expected a string literal for the import path at line " + std::to_string(PeekLine(tokenList, pos)));
	}
	auto pathNode = std::make_unique<StringLiteralNode>();
	pathNode->token = tokenList[pos++];

	if (pos >= tokenList.size() || tokenList[pos].type != TokenType::KwAlias) {
		throw std::runtime_error("Expected 'alias' after import path at line " + std::to_string(PeekLine(tokenList, pos)));
	}
	pos++; // 'alias'

	if (pos >= tokenList.size() || tokenList[pos].type != TokenType::Identifier) {
		throw std::runtime_error("Expected an alias name after 'alias' at line " + std::to_string(PeekLine(tokenList, pos)));
	}
	auto aliasNode = std::make_unique<IdentifierNode>();
	aliasNode->token = tokenList[pos++];

	if (pos >= tokenList.size() || tokenList[pos].type != TokenType::Semicolon) {
		throw std::runtime_error("Expected ';' after import statement at line " + std::to_string(PeekLine(tokenList, pos)));
	}
	pos++; // ';'

	auto importNode = std::make_unique<ImportStmtNode>();
	importNode->token = importToken;
	importNode->children.push_back(std::move(pathNode));
	importNode->children.push_back(std::move(aliasNode));

	return importNode;
}
