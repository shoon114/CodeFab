#include "VarDeclareParser.h"
#include "StatementParserRegistry.h"

namespace {
StatementParserRegistrar<VarDeclareParser> registrar(TokenType::KwVar);
}

std::unique_ptr<SyntaxNode> VarDeclareParser::Parse(const TokenList& tokenList, size_t& pos) {
	const Token& varToken = tokenList[pos++]; // 'var'

	auto varDeclNode = std::make_unique<SyntaxNode>();
	varDeclNode->type = NodeType::VarDeclareStatement;
	varDeclNode->token = varToken;

	// exprParser is responsible for recognizing whether this is a plain
	// identifier ("a") or an assignment ("a = 3") and returning the
	// appropriate tree; VarDeclareParser just attaches whatever comes back.
	auto exprNode = exprParser.Parse(tokenList, pos);
	varDeclNode->children.push_back(std::move(exprNode));

	pos++; // ';'

	return varDeclNode;
}
