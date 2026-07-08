#include "BlockParser.h"
#include "StatementParserRegistry.h"
#include <stdexcept>

namespace {
	StatementParserRegistrar<BlockParser> registrar(TokenType::LBrace);

	// Statement parsers like PrintStatementParser/VarDeclareParser already
	// consume their own trailing ';'. But a bare expression statement inside
	// a block (e.g. "{ a = a + 5; }") resolves directly to ExpressionParser,
	// which -- being reused as a sub-expression parser elsewhere -- must NOT
	// consume a trailing ';' itself. Without this, that leftover ';' would
	// be mistaken for the start of the next statement and fail to resolve.
	// Swallow one optional trailing ';' here so both cases work uniformly.
	void ConsumeOptionalTrailingSemicolon(const TokenList& tokenList, size_t& pos) {
		if (pos < tokenList.size() && tokenList[pos].type == TokenType::Semicolon) {
			pos++;
		}
	}
}

std::unique_ptr<SyntaxNode> BlockParser::Parse(const TokenList& tokenList, size_t& pos) {
	const Token& braceToken = tokenList[pos++]; // '{'

	auto blockNode = std::make_unique<BlockStmtNode>();
	blockNode->token = braceToken;

	while (pos < tokenList.size() && tokenList[pos].type != TokenType::RBrace) {
		std::shared_ptr<IStatementParser> stmtParser =
			StatementParserRegistry::Instance().Resolve(tokenList[pos].type);
		if (stmtParser == nullptr) {
			throw std::runtime_error("Unexpected token inside block");
		}
		blockNode->children.push_back(stmtParser->Parse(tokenList, pos));
		ConsumeOptionalTrailingSemicolon(tokenList, pos);
	}

	if (pos >= tokenList.size() || tokenList[pos].type != TokenType::RBrace) {
		throw std::runtime_error("Expected '}' to close block");
	}

	auto endNode = std::make_unique<BlockStmtNode>();
	endNode->token = tokenList[pos++]; // '}'
	blockNode->children.push_back(std::move(endNode));

	return blockNode;
}
