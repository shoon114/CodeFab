#ifdef _DEBUG
#include "IfStatementParser.h"
#include "StatementParserRegistry.h"

namespace {
StatementParserRegistrar<IfStatementParser> registrar(TokenType::KwIf);
}

std::unique_ptr<SyntaxNode> IfStatementParser::Parse(const TokenList& tokenList, size_t& pos) {
	const Token& ifToken = tokenList[pos++]; // 'if'

	if (tokenList[pos].type != TokenType::LParen) {
		throw std::runtime_error("Invalid Syntax. '(' is Missing");
	}
	pos++; // '('

	auto conditionNode = exprParser.Parse(tokenList, pos);

	if (tokenList[pos].type != TokenType::RParen) {
		throw std::runtime_error("Invalid Syntax. ')' is Missing");
	}
	pos++; // ')'

	auto ifNode = std::make_unique<SyntaxNode>();
	ifNode->type = NodeType::IfStmt;
	ifNode->token = ifToken;
	ifNode->children.push_back(std::move(conditionNode));

	return ifNode;
}
#endif