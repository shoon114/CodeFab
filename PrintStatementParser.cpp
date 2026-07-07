#ifdef _DEBUG
#include "PrintStatementParser.h"
#include "StatementParserRegistry.h"

namespace {
StatementParserRegistrar<PrintStatementParser> registrar(TokenType::Print);
}

std::unique_ptr<SyntaxNode> PrintStatementParser::Parse(const TokenList& tokenList, size_t& pos) {
	const Token& printToken = tokenList[pos++]; // 'print'

	auto exprNode = exprParser.Parse(tokenList, pos);

	pos++; // ';'

	auto printNode = std::make_unique<SyntaxNode>();
	printNode->type = NodeType::PrintStmt;
	printNode->token = printToken;
	printNode->children.push_back(std::move(exprNode));

	return printNode;
}
#endif