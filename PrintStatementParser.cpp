#include "PrintStatementParser.h"
#include "StatementParserRegistry.h"
#include <stdexcept>

namespace {
StatementParserRegistrar<PrintStatementParser> registrar(TokenType::Print);
}

std::unique_ptr<SyntaxNode> PrintStatementParser::Parse(const TokenList& tokenList, size_t& pos) {
	const Token& printToken = tokenList[pos++]; // 'print'

	std::shared_ptr<IStatementParser> exprParser = StatementParserRegistry::Instance().Resolve(tokenList[pos].type);
	if (exprParser == nullptr) {
		throw std::runtime_error("Expected an expression after 'print'");
	}
	auto exprNode = exprParser->Parse(tokenList, pos);

	pos++; // ';'

	auto printNode = std::make_unique<PrintStmtNode>();
	printNode->token = printToken;
	printNode->children.push_back(std::move(exprNode));

	return printNode;
}
