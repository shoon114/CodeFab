#ifdef _DEBUG
#include <stdexcept>
#include <string>
#include "IfStatementParser.h"
#include "StatementParserRegistry.h"

namespace {
	StatementParserRegistrar<IfStatementParser> registrar(TokenType::KwIf);

	void ExpectToken(const TokenList& tokenList, size_t& pos, TokenType expected, const char* symbol) {
		if (tokenList[pos].type != expected) {
			throw std::runtime_error(std::string("Invalid Syntax. '") + symbol + "' is Missing");
		}
		pos++;
	}
}

std::unique_ptr<SyntaxNode> IfStatementParser::Parse(const TokenList& tokenList, size_t& pos) {
	const Token& ifToken = tokenList[pos++]; // 'if'

	ExpectToken(tokenList, pos, TokenType::LParen, "(");
	auto conditionNode = exprParser.Parse(tokenList, pos);
	ExpectToken(tokenList, pos, TokenType::RParen, ")");

	auto ifNode = std::make_unique<SyntaxNode>();
	ifNode->type = NodeType::IfStmt;
	ifNode->token = ifToken;
	ifNode->children.push_back(std::move(conditionNode));

	return ifNode;
}
#endif