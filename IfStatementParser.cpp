#include <stdexcept>
#include <string>
#include "IfStatementParser.h"
#include "StatementParserRegistry.h"

namespace {
	StatementParserRegistrar<IfStatementParser> registrar(TokenType::KwIf);

	// 범위를 벗어나면 EndOfFile/마지막 토큰의 line을 돌려줘서, 매번
	// pos >= tokenList.size()를 따로 검사하지 않아도 되게 한다.
	// (ExpressionParser.cpp/FunctionStatementParser.cpp의 PeekType과 동일한 패턴)
	TokenType PeekType(const TokenList& tokenList, size_t pos) {
		return pos < tokenList.size() ? tokenList[pos].type : TokenType::EndOfFile;
	}

	int PeekLine(const TokenList& tokenList, size_t pos) {
		return pos < tokenList.size() ? tokenList[pos].line : tokenList.back().line;
	}

	std::unique_ptr<SyntaxNode> ResolveAndParse(const TokenList& tokenList, size_t& pos, const char* what) {
		std::shared_ptr<IStatementParser> parser = StatementParserRegistry::Instance().Resolve(PeekType(tokenList, pos));
		if (parser == nullptr) {
			throw std::runtime_error(std::string("Expected ") + what + " at line " + std::to_string(PeekLine(tokenList, pos)));
		}
		return parser->Parse(tokenList, pos);
	}

	void ExpectToken(const TokenList& tokenList, size_t& pos, TokenType expected, const char* what) {
		if (PeekType(tokenList, pos) != expected) {
			throw std::runtime_error(std::string("Expected ") + what + " at line " + std::to_string(PeekLine(tokenList, pos)));
		}
		pos++;
	}
}

std::unique_ptr<SyntaxNode> IfStatementParser::Parse(const TokenList& tokenList, size_t& pos) {
	const Token& ifToken = tokenList[pos++]; // 'if'

	ExpectToken(tokenList, pos, TokenType::LParen, "'(' after 'if'");

	auto conditionNode = ResolveAndParse(tokenList, pos, "a condition expression in 'if'");

	ExpectToken(tokenList, pos, TokenType::RParen, "')' after if-condition");

	if (PeekType(tokenList, pos) != TokenType::LBrace) {
		throw std::runtime_error("Expected '{' to start if-body at line " + std::to_string(PeekLine(tokenList, pos)));
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
