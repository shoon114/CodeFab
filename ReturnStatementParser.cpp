#include "ReturnStatementParser.h"
#include "StatementParserRegistry.h"
#include <stdexcept>
#include <string>

namespace {
StatementParserRegistrar<ReturnStatementParser> registrar(TokenType::KwReturn);
}

std::unique_ptr<SyntaxNode> ReturnStatementParser::Parse(const TokenList& tokenList, size_t& pos) {
	const Token& returnToken = tokenList[pos++];

	auto returnNode = std::make_unique<ReturnStmtNode>();
	returnNode->token = returnToken;

	const std::string missingSemicolonMessage =
		"Expected ';' after return statement at line " + std::to_string(returnToken.line);

	// return 이후 토큰이 ;일 경우 바로 종료한다.
	if (tokenList[pos].type != TokenType::Semicolon) {
		std::shared_ptr<IStatementParser> exprParser = StatementParserRegistry::Instance().Resolve(tokenList[pos].type);
		if (exprParser == nullptr) {
			throw std::runtime_error(missingSemicolonMessage);
		}
		auto exprNode = exprParser->Parse(tokenList, pos);
		returnNode->children.push_back(std::move(exprNode));
	}

	if (pos >= tokenList.size() || tokenList[pos].type != TokenType::Semicolon) {
		throw std::runtime_error(missingSemicolonMessage);
	}
	pos++;

	return returnNode;
}
