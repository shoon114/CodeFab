#include "ExpressionParser.h"
#include <optional>
#include <stdexcept>

namespace {
	TokenType PeekType(const TokenList& tokenList, size_t pos) {
		if (pos >= tokenList.size()) {
			return TokenType::EndOfFile;
		}
		return tokenList[pos].type;
	}

	// Binding power of each infix/assignment operator. Higher binds tighter.
	// rightAssoc == true means a same-precedence operator to the right is allowed
	// to include this position's right-hand side (used for assignment: a = b = 1).
	struct InfixInfo {
		int precedence;
		bool rightAssoc;
		NodeType nodeType;
	};

	std::optional<InfixInfo> GetInfixInfo(TokenType type) {
		switch (type) {
		case TokenType::Assign:
			return InfixInfo{ 1, true, NodeType::AssignExpr };
		case TokenType::Or:
			return InfixInfo{ 2, false, NodeType::BinaryExpr };
		case TokenType::And:
			return InfixInfo{ 3, false, NodeType::BinaryExpr };
		case TokenType::Eq:
		case TokenType::NotEq:
			return InfixInfo{ 4, false, NodeType::BinaryExpr };
		case TokenType::Lt:
		case TokenType::Gt:
		case TokenType::LtEq:
		case TokenType::GtEq:
			return InfixInfo{ 5, false, NodeType::BinaryExpr };
		case TokenType::Plus:
		case TokenType::Minus:
			return InfixInfo{ 6, false, NodeType::BinaryExpr };
		case TokenType::Star:
		case TokenType::Slash:
		case TokenType::Percent:
			return InfixInfo{ 7, false, NodeType::BinaryExpr };
		default:
			return std::nullopt;
		}
	}

	constexpr int kUnaryPrecedence = 8;
}

std::unique_ptr<SyntaxNode> ExpressionParser::Parse(const TokenList& tokenList, size_t& pos) {
	return ParseExpression(tokenList, pos, 0);
}

std::unique_ptr<SyntaxNode> ExpressionParser::MakeBinary(NodeType type, Token op, std::unique_ptr<SyntaxNode> left, std::unique_ptr<SyntaxNode> right) {
	auto node = std::make_unique<SyntaxNode>();
	node->type = type;
	node->token = op;
	node->children.push_back(std::move(left));
	node->children.push_back(std::move(right));
	return node;
}

std::unique_ptr<SyntaxNode> ExpressionParser::ParseExpression(const TokenList& tokenList, size_t& pos, int minPrecedence) {
	std::unique_ptr<SyntaxNode> left = ParsePrefix(tokenList, pos);

	while (true) {
		std::optional<InfixInfo> info = GetInfixInfo(PeekType(tokenList, pos));
		if (!info || info->precedence < minPrecedence) {
			break;
		}

		Token op = tokenList[pos++];
		int nextMinPrecedence = info->rightAssoc ? info->precedence : info->precedence + 1;
		std::unique_ptr<SyntaxNode> right = ParseExpression(tokenList, pos, nextMinPrecedence);

		left = MakeBinary(info->nodeType, op, std::move(left), std::move(right));
	}

	return left;
}

std::unique_ptr<SyntaxNode> ExpressionParser::ParsePrefix(const TokenList& tokenList, size_t& pos) {
	TokenType type = PeekType(tokenList, pos);

	if (type == TokenType::Not || type == TokenType::Minus) {
		Token op = tokenList[pos++];
		std::unique_ptr<SyntaxNode> operand = ParseExpression(tokenList, pos, kUnaryPrecedence);

		auto node = std::make_unique<SyntaxNode>();
		node->type = NodeType::UnaryExpr;
		node->token = op;
		node->children.push_back(std::move(operand));
		return node;
	}

	return ParsePrimary(tokenList, pos);
}

std::unique_ptr<SyntaxNode> ExpressionParser::ParsePrimary(const TokenList& tokenList, size_t& pos) {
	TokenType type = PeekType(tokenList, pos);

	if (type == TokenType::Number) {
		auto node = std::make_unique<SyntaxNode>();
		node->type = NodeType::NumberLiteral;
		node->token = tokenList[pos++];
		return node;
	}

	if (type == TokenType::String) {
		auto node = std::make_unique<SyntaxNode>();
		node->type = NodeType::StringLiteral;
		node->token = tokenList[pos++];
		return node;
	}

	if (type == TokenType::KwTrue || type == TokenType::KwFalse) {
		auto node = std::make_unique<SyntaxNode>();
		node->type = NodeType::BoolLiteral;
		node->token = tokenList[pos++];
		return node;
	}

	if (type == TokenType::Identifier) {
		auto node = std::make_unique<SyntaxNode>();
		node->type = NodeType::Identifier;
		node->token = tokenList[pos++];

		return node;
	}

	if (type == TokenType::LParen) {
		pos++; // '('
		std::unique_ptr<SyntaxNode> inner = ParseExpression(tokenList, pos, 0);
		pos++; // ')'
		return inner;
	}
}