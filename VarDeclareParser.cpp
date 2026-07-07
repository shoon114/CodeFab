#include "VarDeclareParser.h"
#include "StatementParserRegistry.h"

namespace {
StatementParserRegistrar<VarDeclareParser> registrar(TokenType::KwVar);
}

std::unique_ptr<SyntaxNode> VarDeclareParser::Parse(const TokenList& tokenList, size_t& pos) {
	const Token& varToken = tokenList[pos++];
	const Token& identToken = tokenList[pos++];

	auto identNode = std::make_unique<SyntaxNode>();
	identNode->type = NodeType::Identifier;
	identNode->token = identToken;

	auto varDeclNode = std::make_unique<SyntaxNode>();
	varDeclNode->type = NodeType::VarDeclareStatement;
	varDeclNode->token = varToken;

	if (tokenList[pos].type == TokenType::Semicolon) {
		pos++; // ';'
		varDeclNode->children.push_back(std::move(identNode));
		return varDeclNode;
	}

	pos++; // '='
	auto valueNode = exprParser.Parse(tokenList, pos);
	pos++; // ';'

	auto assignNode = std::make_unique<SyntaxNode>();
	assignNode->type = NodeType::AssignExpr;
	assignNode->children.push_back(std::move(identNode));
	assignNode->children.push_back(std::move(valueNode));

	varDeclNode->children.push_back(std::move(assignNode));
	return varDeclNode;
}
