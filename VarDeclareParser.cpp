#include "VarDeclareParser.h"
#include "StatementParserRegistry.h"
#include <stdexcept>

namespace {
StatementParserRegistrar<VarDeclareParser> registrar(TokenType::KwVar);
}

std::unique_ptr<SyntaxNode> VarDeclareParser::Parse(const TokenList& tokenList, size_t& pos) {
	const Token& varToken = tokenList[pos++]; // 'var'

	auto varDeclNode = std::make_unique<VarDeclareStatementNode>();
	varDeclNode->token = varToken;

	if (pos >= tokenList.size()) {
		throw std::runtime_error("Unexpected end of input after 'var'");
	}

	// Resolve whatever statement parser is registered for the token right
	// after 'var' (e.g. ExpressionParser registered for Identifier) instead
	// of depending on a specific parser type directly. This lets new
	// statement forms after 'var' be supported purely by registering them
	// elsewhere, without VarDeclareParser knowing about them.
	std::shared_ptr<IStatementParser> stmtParser = StatementParserRegistry::Instance().Resolve(tokenList[pos].type);
	if (stmtParser == nullptr) {
		throw std::runtime_error("Expected a variable name after 'var' at line " + std::to_string(tokenList[pos].line));
	}

	Token declaredNameToken = tokenList[pos];
	auto childNode = stmtParser->Parse(tokenList, pos);
	varDeclNode->token = declaredNameToken;
	varDeclNode->children.push_back(std::move(childNode));

	if (pos >= tokenList.size() || tokenList[pos].type != TokenType::Semicolon) {
		throw std::runtime_error("Expected ';' after variable declaration at line " + std::to_string(varToken.line));
	}
	pos++; // ';'

	return varDeclNode;
}
