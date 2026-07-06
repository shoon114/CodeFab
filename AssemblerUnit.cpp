#include "AssemblerUnit.h"

std::unique_ptr<SyntaxNode> AssemblerUnit::Parse(const TokenList& tokenList) {
	if (tokenList.empty()) {
		return nullptr;
	}

	size_t pos = 0;
	if (tokenList[pos].type == TokenType::KwVar) {
		return ParseVarDeclare(tokenList, pos);
	}

	return nullptr;
}

std::unique_ptr<SyntaxNode> AssemblerUnit::ParseVarDeclare(const TokenList& tokenList, size_t& pos) {
	const Token& varToken = tokenList[pos++];
	const Token& identToken = tokenList[pos++];
	pos++; // '='
	const Token& valueToken = tokenList[pos++];
	pos++; // ';'

	auto identNode = std::make_unique<SyntaxNode>();
	identNode->type = NodeType::Identifier;
	identNode->token = identToken;

	auto valueNode = std::make_unique<SyntaxNode>();
	valueNode->type = NodeType::NumberLiteral;
	valueNode->token = valueToken;

	auto assignNode = std::make_unique<SyntaxNode>();
	assignNode->type = NodeType::AssignExpr;
	assignNode->children.push_back(std::move(identNode));
	assignNode->children.push_back(std::move(valueNode));

	auto varDeclNode = std::make_unique<SyntaxNode>();
	varDeclNode->type = NodeType::VarDeclareStatement;
	varDeclNode->token = varToken;
	varDeclNode->children.push_back(std::move(assignNode));

	return varDeclNode;
}
