#include <stdexcept>
#include <string>
#include "IfStatementParser.h"
#include "StatementParserRegistry.h"

namespace {
	StatementParserRegistrar<IfStatementParser> registrar(TokenType::KwIf);

	std::unique_ptr<SyntaxNode> ResolveAndParse(const TokenList& tokenList, size_t& pos, const char* what) {
		std::shared_ptr<IStatementParser> parser = StatementParserRegistry::Instance().Resolve(tokenList[pos].type);
		if (parser == nullptr) {
			throw std::runtime_error(std::string("Expected ") + what + " at line " + std::to_string(tokenList[pos].line));
		}
		return parser->Parse(tokenList, pos);
	}

	void ExpectToken(const TokenList& tokenList, size_t& pos, TokenType expected, const char* what) {
		if (pos >= tokenList.size() || tokenList[pos].type != expected) {
			int line = pos < tokenList.size() ? tokenList[pos].line : tokenList.back().line;
			throw std::runtime_error(std::string("Expected ") + what + " at line " + std::to_string(line));
		}
		pos++;
	}
}

std::unique_ptr<SyntaxNode> IfStatementParser::Parse(const TokenList& tokenList, size_t& pos) {
	const Token& ifToken = tokenList[pos++]; // 'if'

	ExpectToken(tokenList, pos, TokenType::LParen, "'(' after 'if'");

	auto conditionNode = ResolveAndParse(tokenList, pos, "a condition expression in 'if'");

	ExpectToken(tokenList, pos, TokenType::RParen, "')' after if-condition");

	if (pos >= tokenList.size() || tokenList[pos].type != TokenType::LBrace) {
		int line = pos < tokenList.size() ? tokenList[pos].line : tokenList.back().line;
		throw std::runtime_error("Expected '{' to start if-body at line " + std::to_string(line));
	}
	// ForStmtParser가 for-loop 본문을 '{'로 강제한 뒤 등록된 파서(BlockParser)에게
	// 위임하는 것과 동일한 방식. if도 중괄호 없는 단일 문장은 지원하지 않는다.
	auto thenNode = ResolveAndParse(tokenList, pos, "'{' to start if-body");

	auto ifNode = std::make_unique<IfStmtNode>();
	ifNode->token = ifToken;
	ifNode->children.push_back(std::move(conditionNode));
	ifNode->children.push_back(std::move(thenNode));

	return ifNode;
}
