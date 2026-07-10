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

	// Tokenizer가 import한 파일의 토큰을 이 자리에 그대로 이어붙이고 그 끝에
	// ImportEnd 마커를 심어둔다. 그 마커를 만날 때까지 문장을 계속 파싱해
	// importNode의 children(children[2:])으로 붙인다 — 이렇게 해야 뒤이어 같은
	// 문장 목록에 오는 사용자 코드와 import 내용이 섞이지 않는다.
	while (pos < tokenList.size() && tokenList[pos].type != TokenType::ImportEnd) {
		auto parser = StatementParserRegistry::Instance().Resolve(tokenList[pos].type);
		if (!parser) {
			throw std::runtime_error("Unexpected token while parsing imported content at line " + std::to_string(PeekLine(tokenList, pos)));
		}
		importNode->children.push_back(parser->Parse(tokenList, pos));
	}

	if (pos >= tokenList.size() || tokenList[pos].type != TokenType::ImportEnd) {
		throw std::runtime_error("Missing import boundary marker at line " + std::to_string(PeekLine(tokenList, pos)));
	}
	pos++; // ImportEnd

	return importNode;
}
