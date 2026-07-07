#include "ExecutorUnit.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

void ExecutorUnit::Execute(const SyntaxNode& tree) {
	if (tree.type != NodeType::Program) {
		return;
	}

	scopes.clear();
	EnterScope(); // Global 스코프

	for (const auto& stmt : tree.children) {
		ExecuteStmt(*stmt);
	}
}

void ExecutorUnit::EnterScope() {
	scopes.emplace_back();
}

void ExecutorUnit::ExitScope() {
	scopes.pop_back();
}

Value_t& ExecutorUnit::ResolveVariable(const std::string& name, int line) {
	for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
		auto found = it->find(name);
		if (found != it->end()) {
			return found->second;
		}
	}
	throw std::runtime_error("Undefined variable '" + name + "' at line " + std::to_string(line));
}

double ExecutorUnit::AsNumber(const Value_t& value, int line) {
	if (!std::holds_alternative<double>(value)) {
		throw std::runtime_error("Expected a number at line " + std::to_string(line));
	}
	return std::get<double>(value);
}

void ExecutorUnit::ExecuteStmt(const SyntaxNode& node) {
	switch (node.type) {
	case NodeType::PrintStmt: {
		const auto& expression = *node.children[0];
		Value_t value = Evaluate(expression);
		if (std::holds_alternative<std::string>(value)) {
			std::cout << std::get<std::string>(value);
		} else {
			double number = std::get<double>(value);
			if (number == std::floor(number)) {
				std::cout << static_cast<long long>(number);
			} else {
				std::cout << number;
			}
		}
		break;
	}
	case NodeType::VarDeclareStatement: {
		const auto& initializer = *node.children[0];
		scopes.back()[node.token.lexeme] = Evaluate(initializer);
		break;
	}
	case NodeType::BlockStmt: {
		EnterScope();
		for (const auto& stmt : node.children) {
			ExecuteStmt(*stmt);
		}
		ExitScope();
		break;
	}
	case NodeType::IfStmt: {
		const auto& condition = *node.children[0];
		const auto& thenBranch = *node.children[1];
		if (AsNumber(Evaluate(condition), condition.token.line) != 0.0) {
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
		while (AsNumber(Evaluate(condition), condition.token.line) != 0.0) {
			ExecuteStmt(body);
			Evaluate(increment);
		}
		break;
	}
	default:
		break;
	}
}

Value_t ExecutorUnit::Evaluate(const SyntaxNode& node) {
	switch (node.type) {
	case NodeType::NumberLiteral:
		return node.token.realValue;
	case NodeType::StringLiteral:
		return node.token.lexeme;
	case NodeType::Identifier:
		return ResolveVariable(node.token.lexeme, node.token.line);
	case NodeType::AssignExpr: {
		const auto& valueExpr = *node.children[0];
		Value_t value = Evaluate(valueExpr);
		ResolveVariable(node.token.lexeme, node.token.line) = value;
		return value;
	}
	case NodeType::BinaryExpr: {
		const auto& leftExpr = *node.children[0];
		const auto& rightExpr = *node.children[1];
		double left = AsNumber(Evaluate(leftExpr), node.token.line);
		double right = AsNumber(Evaluate(rightExpr), node.token.line);
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
		case TokenType::Gt: return static_cast<double>(left > right);
		case TokenType::Lt: return static_cast<double>(left < right);
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
