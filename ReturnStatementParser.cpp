#include "ReturnStatementParser.h"
#include "StatementParserRegistry.h"

namespace {
StatementParserRegistrar<ReturnStatementParser> registrar(TokenType::KwReturn);
}

std::unique_ptr<SyntaxNode> ReturnStatementParser::Parse(const TokenList& tokenList, size_t& pos) {
	const Token& returnToken = tokenList[pos++];

	auto returnNode = std::make_unique<ReturnStmtNode>();
	returnNode->token = returnToken;

	//return이후 토큰이 ;일 경우 바로 종료한다.
	if (tokenList[pos].type != TokenType::Semicolon) {
		std::shared_ptr<IStatementParser> exprParser = StatementParserRegistry::Instance().Resolve(tokenList[pos].type);
		auto exprNode = exprParser->Parse(tokenList, pos);
		returnNode->children.push_back(std::move(exprNode));
	}

	pos++;

	return returnNode;
}
