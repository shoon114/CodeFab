#include "ExecutorUnit.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

void ExecutorUnit::Execute(const SyntaxNode& tree) {
	if (tree.type != NodeType::Program) {
		return;
	}

	for (const auto& stmt : tree.children) {
		ExecuteStmt(*stmt);
	}
}

void ExecutorUnit::ExecuteStmt(const SyntaxNode& node) {
	switch (node.type) {
	case NodeType::PrintStmt: {
		const SyntaxNode& expression = *node.children[0];
		if (expression.type == NodeType::StringLiteral) {
			std::cout << expression.token.lexeme;
			break;
		}
		double value = Evaluate(expression);
		if (value == std::floor(value)) {
			std::cout << static_cast<long long>(value);
		} else {
			std::cout << value;
		}
		break;
	}
	case NodeType::VarDeclareStatement: {
		const auto& initializer = *node.children[0];
		variables[node.token.lexeme] = Evaluate(initializer);
		break;
	}
	case NodeType::BlockStmt: {
		for (const auto& stmt : node.children) {
			ExecuteStmt(*stmt);
		}
		break;
	}
	case NodeType::IfStmt: {
		const auto& condition = *node.children[0];
		const auto& thenBranch = *node.children[1];
		if (Evaluate(condition) != 0.0) {
			ExecuteStmt(thenBranch);
		}
		break;
	}
	case NodeType::ExprStmt: {
		const auto& expression = *node.children[0];
		Evaluate(expression);
		break;
	}
	case NodeType::ForStmt: {
		const auto& init = *node.children[0];
		const auto& condition = *node.children[1];
		const auto& increment = *node.children[2];
		const auto& body = *node.children[3];
		ExecuteStmt(init);
		while (Evaluate(condition) != 0.0) {
			ExecuteStmt(body);
			Evaluate(increment);
		}
		break;
	}
	default:
		break;
	}
}

double ExecutorUnit::Evaluate(const SyntaxNode& node) {
	switch (node.type) {
	case NodeType::NumberLiteral:
		return node.token.realValue;
	case NodeType::Identifier: {
		auto it = variables.find(node.token.lexeme);
		if (it == variables.end()) {
			throw std::runtime_error(
				"Undefined variable '" + node.token.lexeme + "' at line " +
				std::to_string(node.token.line));
		}
		return it->second;
	}
	case NodeType::AssignExpr: {
		const auto& valueExpr = *node.children[0];
		double value = Evaluate(valueExpr);
		variables[node.token.lexeme] = value;
		return value;
	}
	case NodeType::BinaryExpr: {
		const auto& leftExpr = *node.children[0];
		const auto& rightExpr = *node.children[1];
		double left = Evaluate(leftExpr);
		double right = Evaluate(rightExpr);
		switch (node.token.type) {
		case TokenType::Plus:  return left + right;
		case TokenType::Minus: return left - right;
		case TokenType::Star:  return left * right;
		case TokenType::Slash:
			if (right == 0.0) {
				throw std::runtime_error(
					"Division by zero at line " + std::to_string(node.token.line) +
					", column " + std::to_string(node.token.column));
			}
			return left / right;
		case TokenType::Gt: return left > right;
		case TokenType::Lt: return left < right;
		default:
			throw std::runtime_error(
				"Unsupported binary operator at line " + std::to_string(node.token.line));
		}
	}
	default:
		throw std::runtime_error(
			"Unsupported expression node at line " + std::to_string(node.token.line));
	}
}
