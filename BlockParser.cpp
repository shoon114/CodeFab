#include "BlockParser.h"
#include "StatementParserRegistry.h"
#include <stdexcept>

namespace {
StatementParserRegistrar<BlockParser> registrar(TokenType::LBrace);
}

std::unique_ptr<SyntaxNode> BlockParser::Parse(const TokenList& tokenList, size_t& pos) {
	const Token& braceToken = tokenList[pos++]; // '{'

	auto blockNode = std::make_unique<SyntaxNode>();
	blockNode->type = NodeType::BlockStmt;
	blockNode->token = braceToken;

	while (pos < tokenList.size() && tokenList[pos].type != TokenType::RBrace) {
		std::shared_ptr<IStatementParser> stmtParser =
			StatementParserRegistry::Instance().Resolve(tokenList[pos].type, exprParser);
		blockNode->children.push_back(stmtParser->Parse(tokenList, pos));
	}

	auto endNode = std::make_unique<SyntaxNode>();
	endNode->type = NodeType::BlockStmt;
	endNode->token = tokenList[pos++]; // '}'
	blockNode->children.push_back(std::move(endNode));

	return blockNode;
}
