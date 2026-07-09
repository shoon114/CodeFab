#include "ExpressionParser.h"
#include "StatementParserRegistry.h"
#include <optional>
#include <stdexcept>

namespace {
	// ExpressionParser is registered under every token type that can start
	// an expression (see ParsePrimary/ParsePrefix below), so any other
	// parser can obtain it via StatementParserRegistry::Resolve() without
	// depending on ExpressionParser directly.
	StatementParserRegistrar<ExpressionParser> numberRegistrar(TokenType::Number);
	StatementParserRegistrar<ExpressionParser> stringRegistrar(TokenType::String);
	StatementParserRegistrar<ExpressionParser> identifierRegistrar(TokenType::Identifier);
	StatementParserRegistrar<ExpressionParser> trueRegistrar(TokenType::KwTrue);
	StatementParserRegistrar<ExpressionParser> falseRegistrar(TokenType::KwFalse);
	StatementParserRegistrar<ExpressionParser> lParenRegistrar(TokenType::LParen);
	StatementParserRegistrar<ExpressionParser> thisRegistrar(TokenType::KwThis);
	StatementParserRegistrar<ExpressionParser> superRegistrar(TokenType::KwSuper);
	StatementParserRegistrar<ExpressionParser> minusRegistrar(TokenType::Minus);
	StatementParserRegistrar<ExpressionParser> notRegistrar(TokenType::Not);

	TokenType PeekType(const TokenList& tokenList, size_t pos) {
		if (pos >= tokenList.size()) {
			return TokenType::EndOfFile;
		}
		return tokenList[pos].type;
	}

	// Binding power of each infix/assignment operator. Higher binds tighter.
	// All operators are left-associative.
	struct InfixInfo {
		int precedence;
		NodeType nodeType;
	};

	std::optional<InfixInfo> GetInfixInfo(TokenType type) {
		switch (type) {
		case TokenType::Assign:
			return InfixInfo{ 1, NodeType::AssignExpr };
		case TokenType::Or:
			return InfixInfo{ 2, NodeType::BinaryExpr };
		case TokenType::And:
			return InfixInfo{ 3, NodeType::BinaryExpr };
		case TokenType::Eq:
		case TokenType::NotEq:
			return InfixInfo{ 4, NodeType::BinaryExpr };
		case TokenType::Lt:
		case TokenType::Gt:
		case TokenType::LtEq:
		case TokenType::GtEq:
		case TokenType::KwInstanceof:
			return InfixInfo{ 5, NodeType::BinaryExpr };
		case TokenType::Plus:
		case TokenType::Minus:
			return InfixInfo{ 6, NodeType::BinaryExpr };
		case TokenType::Star:
		case TokenType::Slash:
		case TokenType::Percent:
			return InfixInfo{ 7, NodeType::BinaryExpr };
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
	std::unique_ptr<SyntaxNode> node;
	if (type == NodeType::AssignExpr) {
		node = std::make_unique<AssignExprNode>();
	} else {
		node = std::make_unique<BinaryExprNode>();
	}
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
		std::unique_ptr<SyntaxNode> right = ParseExpression(tokenList, pos, info->precedence + 1);

		left = MakeBinary(info->nodeType, op, std::move(left), std::move(right));
	}

	return left;
}

std::unique_ptr<SyntaxNode> ExpressionParser::ParsePrefix(const TokenList& tokenList, size_t& pos) {
	TokenType type = PeekType(tokenList, pos);

	if (type == TokenType::Not || type == TokenType::Minus) {
		Token op = tokenList[pos++];
		std::unique_ptr<SyntaxNode> operand = ParseExpression(tokenList, pos, kUnaryPrecedence);

		auto node = std::make_unique<UnaryExprNode>();
		node->token = op;
		node->children.push_back(std::move(operand));
		return node;
	}

	return ParsePrimary(tokenList, pos);
}

std::unique_ptr<SyntaxNode> ExpressionParser::ParsePrimary(const TokenList& tokenList, size_t& pos) {
	std::unique_ptr<SyntaxNode> node = ParseAtom(tokenList, pos);

	while (true) {
		TokenType type = PeekType(tokenList, pos);
		if (type == TokenType::LParen) {
			node = ParseCallExpr(tokenList, pos, std::move(node));
		} else if (type == TokenType::Dot) {
			node = ParseMemberAccessExpr(tokenList, pos, std::move(node));
		} else if (type == TokenType::LBracket) {
			node = ParseIndexExpr(tokenList, pos, std::move(node));
		} else {
			break;
		}
	}

	return node;
}

std::unique_ptr<SyntaxNode> ExpressionParser::ParseCallExpr(const TokenList& tokenList, size_t& pos, std::unique_ptr<SyntaxNode> callee) {
	if (callee->type != NodeType::Identifier && callee->type != NodeType::MemberAccessExpr) {
		throw std::runtime_error("Only an identifier or member access can be called at line " + std::to_string(callee->token.line));
	}
	Token calleeToken = callee->token; // function/method name
	pos++; // '('

	if (callee->type == NodeType::Identifier && calleeToken.lexeme == "Array") {
		return ParseArrExpr(tokenList, pos, calleeToken);
	}

	auto node = std::make_unique<CallExprNode>();
	node->token = calleeToken;
	if (callee->type == NodeType::MemberAccessExpr) {
		node->children.push_back(std::move(callee)); // children[0] = MemberAccessExprNode (대상.메서드)
	}

	if (PeekType(tokenList, pos) != TokenType::RParen) {
		node->children.push_back(ParseExpression(tokenList, pos, 0));
		while (PeekType(tokenList, pos) == TokenType::Comma) {
			pos++; // ','
			node->children.push_back(ParseExpression(tokenList, pos, 0));
		}
	}

	if (PeekType(tokenList, pos) != TokenType::RParen) {
		throw std::runtime_error("Expected ')' after arguments at line " + std::to_string(calleeToken.line));
	}
	pos++; // ')'

	return node;
}

std::unique_ptr<SyntaxNode> ExpressionParser::ParseMemberAccessExpr(const TokenList& tokenList, size_t& pos, std::unique_ptr<SyntaxNode> object) {
	pos++; // '.'
	if (PeekType(tokenList, pos) != TokenType::Identifier) {
		throw std::runtime_error("Expected a member name after '.' at line " + std::to_string(object->token.line));
	}
	auto node = std::make_unique<MemberAccessExprNode>();
	node->token = tokenList[pos++]; // member name
	node->children.push_back(std::move(object));
	return node;
}

std::unique_ptr<SyntaxNode> ExpressionParser::ParseArrExpr(const TokenList& tokenList, size_t& pos, Token calleeToken) {
	auto node = std::make_unique<ArrExprNode>();
	node->token = calleeToken;
	node->children.push_back(ParseExpression(tokenList, pos, 0));

	if (PeekType(tokenList, pos) != TokenType::RParen) {
		throw std::runtime_error("Expected ')' after Array size at line " + std::to_string(calleeToken.line));
	}
	pos++; // ')'

	return node;
}

std::unique_ptr<SyntaxNode> ExpressionParser::ParseIndexExpr(const TokenList& tokenList, size_t& pos, std::unique_ptr<SyntaxNode> array) {
	if (array->type != NodeType::Identifier) {
		throw std::runtime_error("Only an identifier can be indexed at line " + std::to_string(array->token.line));
	}
	Token bracketToken = tokenList[pos++]; // '['

	std::unique_ptr<SyntaxNode> indexExpr = ParseExpression(tokenList, pos, 0);
	if (PeekType(tokenList, pos) != TokenType::RBracket) {
		throw std::runtime_error("Expected ']' after index expression at line " + std::to_string(bracketToken.line));
	}
	pos++; // ']'

	auto node = std::make_unique<IndexExprNode>();
	node->token = bracketToken;
	node->children.push_back(std::move(array));
	node->children.push_back(std::move(indexExpr));
	return node;
}

std::unique_ptr<SyntaxNode> ExpressionParser::ParseAtom(const TokenList& tokenList, size_t& pos) {
	TokenType type = PeekType(tokenList, pos);

	if (type == TokenType::Number) {
		auto node = std::make_unique<NumberLiteralNode>();
		node->token = tokenList[pos++];
		return node;
	}

	if (type == TokenType::String) {
		auto node = std::make_unique<StringLiteralNode>();
		node->token = tokenList[pos++];
		return node;
	}

	if (type == TokenType::KwTrue || type == TokenType::KwFalse) {
		auto node = std::make_unique<BoolLiteralNode>();
		node->token = tokenList[pos++];
		return node;
	}

	if (type == TokenType::Identifier) {
		auto node = std::make_unique<IdentifierNode>();
		node->token = tokenList[pos++];

		return node;
	}

	if (type == TokenType::KwThis) {
		auto node = std::make_unique<ThisExprNode>();
		node->token = tokenList[pos++];
		return node;
	}

	if (type == TokenType::KwSuper) {
		auto node = std::make_unique<SuperExprNode>();
		node->token = tokenList[pos++];
		return node;
	}

	if (type == TokenType::LParen) {
		pos++; // '('
		std::unique_ptr<SyntaxNode> inner = ParseExpression(tokenList, pos, 0);
		if (PeekType(tokenList, pos) != TokenType::RParen) {
			throw std::runtime_error("Expected ')' after parenthesized expression");
		}
		pos++; // ')'
		return inner;
	}

	throw std::runtime_error("Unexpected token while parsing expression");
}