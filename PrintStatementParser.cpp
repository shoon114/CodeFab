#include "PrintStatementParser.h"
#include "StatementParserRegistry.h"
#include <stdexcept>
#include <string>

namespace {
StatementParserRegistrar<PrintStatementParser> registrar(TokenType::Print);
}

std::unique_ptr<SyntaxNode> PrintStatementParser::Parse(const TokenList& tokenList, size_t& pos) {
	const Token& printToken = tokenList[pos++]; 

	std::shared_ptr<IStatementParser> exprParser = StatementParserRegistry::Instance().Resolve(tokenList[pos].type);
	if (exprParser == nullptr) {
		throw std::runtime_error("Expected an expression after 'print'");
	}
	auto exprNode = exprParser->Parse(tokenList, pos);

	if (pos >= tokenList.size() || tokenList[pos].type != TokenType::Semicolon) {
		int line = pos < tokenList.size() ? tokenList[pos].line : tokenList.back().line;
		throw std::runtime_error("Expected ';' after value at line " + std::to_string(line));
	}
	pos++; // ';'

	auto printNode = std::make_unique<PrintStmtNode>();
	printNode->token = printToken;
	printNode->children.push_back(std::move(exprNode));

	return printNode;
}
