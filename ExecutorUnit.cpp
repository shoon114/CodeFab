#include "ExecutorUnit.h"
#include <cmath>
#include <iostream>

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
	case NodeType::ExprStmt:
		Evaluate(*node.children[0]);
		break;
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
		case TokenType::Slash: return left / right;
		default: return 0.0;
		}
	}
	default:
		return 0.0;
	}
}
