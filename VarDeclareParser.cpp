#include "VarDeclareParser.h"
#include "StatementParserRegistry.h"
#include <stdexcept>

namespace {
StatementParserRegistrar<VarDeclareParser> registrar(TokenType::KwVar);
}

std::unique_ptr<SyntaxNode> VarDeclareParser::Parse(const TokenList& tokenList, size_t& pos) {
	const Token& varToken = tokenList[pos++]; // 'var'

	auto varDeclNode = std::make_unique<SyntaxNode>();
	varDeclNode->type = NodeType::VarDeclareStatement;
	varDeclNode->token = varToken;

	if (pos >= tokenList.size()) {
		throw std::runtime_error("Unexpected end of input after 'var'");
	}

	// Resolve whatever statement parser is registered for the token right
	// after 'var' (e.g. an ExprStmtParser registered for Identifier, which
	// itself decides whether this is a plain identifier or an assignment
	// via exprParser) instead of calling exprParser directly. This lets new
	// statement forms after 'var' be supported purely by registering them
	// elsewhere, without VarDeclareParser knowing about them.
	std::shared_ptr<IStatementParser> stmtParser = StatementParserRegistry::Instance().Resolve(tokenList[pos].type, exprParser);
	auto childNode = stmtParser->Parse(tokenList, pos);
	varDeclNode->children.push_back(std::move(childNode));

	pos++; // ';'

	return varDeclNode;
}
