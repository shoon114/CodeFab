#ifdef _DEBUG
#include "IfStatementParser.h"
#include "StatementParserRegistry.h"

namespace {
StatementParserRegistrar<IfStatementParser> registrar(TokenType::KwIf);
}

std::unique_ptr<SyntaxNode> IfStatementParser::Parse(const TokenList& tokenList, size_t& pos) {
	const Token& ifToken = tokenList[pos++]; // 'if'
	pos++; // '('
	auto conditionNode = exprParser.Parse(tokenList, pos);
	pos++; // ')'

	auto ifNode = std::make_unique<SyntaxNode>();
	ifNode->type = NodeType::IfStmt;
	ifNode->token = ifToken;
	ifNode->children.push_back(std::move(conditionNode));

	return ifNode;
}
#endif