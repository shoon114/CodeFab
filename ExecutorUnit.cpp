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
	default:
		break;
	}
}

double ExecutorUnit::Evaluate(const SyntaxNode& node) {
	switch (node.type) {
	case NodeType::NumberLiteral:
		return node.token.realValue;
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
		default: return 0.0;
		}
	}
	default:
		return 0.0;
	}
}
