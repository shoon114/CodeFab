#include "ReturnStatementParser.h"
#include "StatementParserRegistry.h"

namespace {
StatementParserRegistrar<ReturnStatementParser> registrar(TokenType::KwReturn);
}

std::unique_ptr<SyntaxNode> ReturnStatementParser::Parse(const TokenList& tokenList, size_t& pos) {
	const Token& returnToken = tokenList[pos++]; // 'return'

	std::shared_ptr<IStatementParser> exprParser = StatementParserRegistry::Instance().Resolve(tokenList[pos].type);
	auto exprNode = exprParser->Parse(tokenList, pos);

	pos++; // ';'

	auto returnNode = std::make_unique<ReturnStmtNode>();
	returnNode->token = returnToken;
	returnNode->children.push_back(std::move(exprNode));

	return returnNode;
}
