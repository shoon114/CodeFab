#include "ExecutorUnit.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

void ExecutorUnit::Execute(const SyntaxNode& tree) {
	if (tree.type != NodeType::Program) {
		return;
	}

	if (scopes.empty()) {
		EnterScope(); // Global 스코프(최초 1회만 생성 — 이후 호출에서도 유지되어야 REPL에서 이전에
		// 선언한 변수를 계속 참조할 수 있다)
	}

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
	throw std::runtime_error("[" + std::to_string(line) + "번째줄] 미정의된 변수 '" + name + "'");
}

double ExecutorUnit::AsNumber(const Value_t& value, int line) {
	if (!std::holds_alternative<double>(value)) {
		throw std::runtime_error("[" + std::to_string(line) + "번째줄] 피연산자는 반드시 숫자여야 한다.");
	}
	return std::get<double>(value);
}

void ExecutorUnit::EnsureNonZeroDivisor(double divisor, int line) {
	if (divisor == 0.0) {
		throw std::runtime_error("[" + std::to_string(line) + "번째줄] 0으로 나눈 오류");
	}
}

bool ExecutorUnit::IsTruthy(const Value_t& value) {
	if (std::holds_alternative<bool>(value)) {
		return std::get<bool>(value);
	}
	if (std::holds_alternative<double>(value)) {
		return std::get<double>(value) != 0.0;
	}
	return !std::get<std::string>(value).empty();
}

void ExecutorUnit::ExecuteStmt(const SyntaxNode& node) {
	node.Accept(*this);
}

Value_t ExecutorUnit::Evaluate(const SyntaxNode& node) {
	node.Accept(*this);
	return lastValue;
}

void ExecutorUnit::Visit(const PrintStmtNode& node) { ExecutePrintStmt(node); }
void ExecutorUnit::Visit(const VarDeclareStatementNode& node) { ExecuteVarDeclareStatement(node); }
void ExecutorUnit::Visit(const BlockStmtNode& node) { ExecuteBlockStmt(node); }
void ExecutorUnit::Visit(const IfStmtNode& node) { ExecuteIfStmt(node); }
void ExecutorUnit::Visit(const ExprStmtNode& node) { ExecuteExprStmt(node); }
void ExecutorUnit::Visit(const ForStmtNode& node) { ExecuteForStmt(node); }

// 함수는 아직 실행 단계에서 지원하지 않는다(체커 단계까지만 검증 대상).
void ExecutorUnit::Visit(const FuncDeclStmtNode&) {}
void ExecutorUnit::Visit(const ReturnStmtNode&) {}

void ExecutorUnit::Visit(const NumberLiteralNode& node) { lastValue = node.token.realValue; }
void ExecutorUnit::Visit(const StringLiteralNode& node) { lastValue = node.token.lexeme; }
void ExecutorUnit::Visit(const BoolLiteralNode& node) { lastValue = (node.token.lexeme == "true"); }
void ExecutorUnit::Visit(const IdentifierNode& node) { lastValue = EvaluateIdentifier(node); }
void ExecutorUnit::Visit(const AssignExprNode& node) { lastValue = EvaluateAssignExpr(node); }
void ExecutorUnit::Visit(const BinaryExprNode& node) { lastValue = EvaluateBinaryExpr(node); }
void ExecutorUnit::Visit(const UnaryExprNode& node) { lastValue = EvaluateUnaryExpr(node); }

void ExecutorUnit::Visit(const CallExprNode& node) {
	throw std::runtime_error("함수 호출은 아직 지원되지 않습니다 at line " + std::to_string(node.token.line));
}

void ExecutorUnit::ExecutePrintStmt(const SyntaxNode& node) {
	const auto& expression = *node.children[0];
	Value_t value = Evaluate(expression);
	if (std::holds_alternative<std::string>(value)) {
		std::cout << std::get<std::string>(value);
	} else if (std::holds_alternative<bool>(value)) {
		std::cout << (std::get<bool>(value) ? "true" : "false");
	} else {
		double number = std::get<double>(value);
		if (number == std::floor(number)) {
			std::cout << static_cast<long long>(number);
		} else {
			std::cout << number;
		}
	}
}

void ExecutorUnit::ExecuteVarDeclareStatement(const SyntaxNode& node) {
	// VarDeclareParser는 초기화식이 있으면 children[0]에 AssignExpr(Identifier, value)를,
	// 초기화식이 없으면 children[0]에 Identifier만 둔다.
	const auto& declared = *node.children[0];
	if (declared.type == NodeType::AssignExpr) {
		const auto& identifier = *declared.children[0];
		const auto& initializer = *declared.children[1];
		scopes.back()[identifier.token.lexeme] = Evaluate(initializer);
	} else {
		scopes.back()[declared.token.lexeme] = 0.0;
	}
}

void ExecutorUnit::ExecuteBlockStmt(const SyntaxNode& node) {
	EnterScope();
	for (const auto& stmt : node.children) {
		ExecuteStmt(*stmt);
	}
	ExitScope();
}

void ExecutorUnit::ExecuteIfStmt(const SyntaxNode& node) {
	const auto& condition = *node.children[0];
	const auto& thenBranch = *node.children[1];
	if (IsTruthy(Evaluate(condition))) {
		ExecuteStmt(thenBranch);
	}
}

void ExecutorUnit::ExecuteExprStmt(const SyntaxNode& node) {
	const auto& expression = *node.children[0];
	Evaluate(expression);
}

void ExecutorUnit::ExecuteForStmt(const SyntaxNode& node) {
	const auto& init = *node.children[0];
	const auto& condition = *node.children[1];
	const auto& increment = *node.children[2];
	const auto& body = *node.children[3];
	ExecuteStmt(init);
	while (IsTruthy(Evaluate(condition))) {
		ExecuteStmt(body);
		Evaluate(increment);
	}
}

Value_t ExecutorUnit::EvaluateIdentifier(const SyntaxNode& node) {
	return ResolveVariable(node.token.lexeme, node.token.line);
}

Value_t ExecutorUnit::EvaluateAssignExpr(const SyntaxNode& node) {
	// ExpressionParser는 AssignExpr의 token을 '=' 연산자로, children을
	// [Identifier, valueExpr] 순서로 둔다.
	const auto& identifier = *node.children[0];
	const auto& valueExpr = *node.children[1];
	Value_t value = Evaluate(valueExpr);
	ResolveVariable(identifier.token.lexeme, node.token.line) = value;
	return value;
}

Value_t ExecutorUnit::EvaluateBinaryExpr(const SyntaxNode& node) {
	const auto& leftExpr = *node.children[0];
	const auto& rightExpr = *node.children[1];

	// And/Or는 왼쪽만 보고 결과가 정해지면 오른쪽을 평가하지 않는다(단락 평가).
	if (node.token.type == TokenType::And) {
		if (!IsTruthy(Evaluate(leftExpr))) {
			return false;
		}
		return IsTruthy(Evaluate(rightExpr));
	}
	if (node.token.type == TokenType::Or) {
		if (IsTruthy(Evaluate(leftExpr))) {
			return true;
		}
		return IsTruthy(Evaluate(rightExpr));
	}

	Value_t leftValue = Evaluate(leftExpr);
	Value_t rightValue = Evaluate(rightExpr);

	if (node.token.type == TokenType::Plus &&
		std::holds_alternative<std::string>(leftValue) &&
		std::holds_alternative<std::string>(rightValue)) {
		return std::get<std::string>(leftValue) + std::get<std::string>(rightValue);
	}

	// Eq/NotEq는 숫자로 강제 변환하지 않고 값(숫자/문자열/불리언) 자체를 비교한다.
	if (node.token.type == TokenType::Eq) {
		return leftValue == rightValue;
	}
	if (node.token.type == TokenType::NotEq) {
		return !(leftValue == rightValue);
	}

	double left = AsNumber(leftValue, node.token.line);
	double right = AsNumber(rightValue, node.token.line);
	switch (node.token.type) {
	case TokenType::Plus:  return left + right;
	case TokenType::Minus: return left - right;
	case TokenType::Star:  return left * right;
	case TokenType::Slash:
		EnsureNonZeroDivisor(right, node.token.line);
		return left / right;
	case TokenType::Percent:
		EnsureNonZeroDivisor(right, node.token.line);
		return std::fmod(left, right);
	case TokenType::Gt:   return left > right;
	case TokenType::Lt:   return left < right;
	case TokenType::GtEq: return left >= right;
	case TokenType::LtEq: return left <= right;
	default:
		throw std::runtime_error(
			"Unsupported binary operator at line " + std::to_string(node.token.line));
	}
}

Value_t ExecutorUnit::EvaluateUnaryExpr(const SyntaxNode& node) {
	const auto& operand = *node.children[0];
	Value_t value = Evaluate(operand);
	switch (node.token.type) {
	case TokenType::Minus:
		return -AsNumber(value, node.token.line);
	case TokenType::Not:
		return !IsTruthy(value);
	default:
		throw std::runtime_error(
			"Unsupported unary operator at line " + std::to_string(node.token.line));
	}
}
