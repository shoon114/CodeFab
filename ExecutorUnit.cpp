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
		double value = Evaluate(*node.children[0]);
		if (value == std::floor(value)) {
			std::cout << static_cast<long long>(value) << std::endl;
		} else {
			std::cout << value << std::endl;
		}
		break;
	}
	case NodeType::VarDeclStmt: {
		variables[node.token.lexeme] = Evaluate(*node.children[0]);
		break;
	}
	case NodeType::BlockStmt: {
		for (const auto& stmt : node.children) {
			ExecuteStmt(*stmt);
		}
		break;
	}
	case NodeType::IfStmt: {
		double condition = Evaluate(*node.children[0]);
		if (condition != 0.0) {
			ExecuteStmt(*node.children[1]);
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
	case NodeType::BinaryExpr: {
		double left = Evaluate(*node.children[0]);
		double right = Evaluate(*node.children[1]);
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
		case TokenType::Gt: return left > right ? true : false;
		default: return 0.0;
		}
	}
	default:
		return 0.0;
	}
}
