#include "ExecutorUnit.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
	// return문 실행을 표현하는 신호. InvokeMethod가 메서드 본문을 실행하다가 이 예외를
	// 잡으면 그 지점에서 실행을 멈추고 값을 반환값으로 쓴다. 중첩된 if/for 안에서
	// return을 만나도, 기존 ExecuteBlockStmt/ExecuteIfStmt/ExecuteForStmt를 전혀
	// 건드리지 않고 C++ 예외 전파만으로 자연스럽게 그 블록들을 빠져나가게 된다.
	struct ReturnSignal {
		Value_t value;
	};
}

void ExecutorUnit::Execute(const SyntaxNode& tree) {
	if (tree.type != NodeType::Program) {
		return;
	}

	if (scopes.empty()) {
		EnterScope(); // Global 스코프(최초 1회만 생성 — 이후 호출에서도 유지되어야 REPL에서 이전에
		// 선언한 변수를 계속 참조할 수 있다)
	}

	for (const auto& statement : tree.children) {
		ExecuteStmt(*statement);
	}
}

void ExecutorUnit::EnterScope() {
	scopes.emplace_back();
}

void ExecutorUnit::ExitScope() {
	scopes.pop_back();
}

Value_t& ExecutorUnit::ResolveVariable(const std::string& name, int distance, int line) {
	if (distance >= 0 && static_cast<size_t>(distance) < scopes.size()) {
		size_t index = scopes.size() - 1 - static_cast<size_t>(distance);
		auto found = scopes[index].find(name);
		if (found != scopes[index].end()) {
			return found->second;
		}
	}
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
		throw std::runtime_error("Operand must be a number at line " + std::to_string(line));
	}
	return std::get<double>(value);
}

void ExecutorUnit::EnsureNonZeroDivisor(double divisor, int line) {
	if (divisor == 0.0) {
		throw std::runtime_error("Division by zero at line " + std::to_string(line));
	}
}

bool ExecutorUnit::IsTruthy(const Value_t& value) {
	if (std::holds_alternative<bool>(value)) {
		return std::get<bool>(value);
	}
	if (std::holds_alternative<double>(value)) {
		return std::get<double>(value) != 0.0;
	}
	if (std::holds_alternative<std::monostate>(value)) {
		return false;
	}
	if (std::holds_alternative<std::string>(value)) {
		return !std::get<std::string>(value).empty();
	}
	return true; // 함수/배열 값은 항상 참으로 취급한다.
}

void ExecutorUnit::ExecuteStmt(const SyntaxNode& node) {
	if (observer) observer->OnStmtEnter(node);
	node.Accept(*this);
	if (observer) observer->OnStmtExit(node);
}

void ExecutorUnit::SetObserver(ExecutionObserver* newObserver) {
	observer = newObserver;
}

bool ExecutorUnit::TryGetVariable(const std::string& name, Value_t& outValue) const {
	for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
		auto found = it->find(name);
		if (found != it->end()) {
			outValue = found->second;
			return true;
		}
	}
	return false;
}

const std::unordered_map<std::string, Value_t>& ExecutorUnit::CurrentScope() const {
	return scopes.back();
}

Value_t ExecutorUnit::Evaluate(const SyntaxNode& node) {
	node.Accept(*this);
	return lastValue;
}

ExecutorUnit::ScopeGuard::ScopeGuard(ExecutorUnit& owner) : owner(owner) {
	owner.EnterScope();
}

ExecutorUnit::ScopeGuard::~ScopeGuard() {
	owner.ExitScope();
}

ExecutorUnit::MethodContextGuard::MethodContextGuard(ExecutorUnit& owner, std::shared_ptr<InstanceObject> instance,
	std::string owningClassName) : owner(owner) {
	owner.thisStack.push_back(std::move(instance));
	owner.currentClassNameStack.push_back(std::move(owningClassName));
}

ExecutorUnit::MethodContextGuard::~MethodContextGuard() {
	owner.currentClassNameStack.pop_back();
	owner.thisStack.pop_back();
}

ExecutorUnit::FunctionCallFrameGuard::FunctionCallFrameGuard(ExecutorUnit& owner) : owner(owner) {
	savedLocalFrames.assign(
		std::make_move_iterator(owner.scopes.begin() + 1),
		std::make_move_iterator(owner.scopes.end()));
	owner.scopes.resize(1);
	owner.EnterScope(); // 파라미터 전용 스코프
}

ExecutorUnit::FunctionCallFrameGuard::~FunctionCallFrameGuard() {
	owner.scopes.resize(1); // 예외로 중첩 스코프가 정리 안 된 채 남아있어도 강제로 global까지만 남김
	for (auto& frame : savedLocalFrames) {
		owner.scopes.push_back(std::move(frame));
	}
}

Value_t ExecutorUnit::ExecuteFunctionBody(const SyntaxNode& body) {
	try {
		ExecuteStmt(body);
	} catch (const ReturnSignal& signal) {
		return signal.value;
	}
	return Value_t{ std::monostate{} };
}

// import 모듈 내용을 실행하는 동안(suppressActionsDuringImport)에는 선언이 아닌 "동작"이므로
// 건너뛴다 — PDF 명세상 import는 파일의 선언만 들여오고, print/if/for/단독 식 같은 부수효과는
// 실행하지 않는다.
void ExecutorUnit::Visit(const PrintStmtNode& node) { if (suppressActionsDuringImport) return; ExecutePrintStmt(node); }
void ExecutorUnit::Visit(const VarDeclareStatementNode& node) { ExecuteVarDeclareStatement(node); }
void ExecutorUnit::Visit(const BlockStmtNode& node) { if (suppressActionsDuringImport) return; ExecuteBlockStmt(node); }
void ExecutorUnit::Visit(const IfStmtNode& node) { if (suppressActionsDuringImport) return; ExecuteIfStmt(node); }
void ExecutorUnit::Visit(const ExprStmtNode& node) { if (suppressActionsDuringImport) return; ExecuteExprStmt(node); }
void ExecutorUnit::Visit(const ForStmtNode& node) { if (suppressActionsDuringImport) return; ExecuteForStmt(node); }

// 최상위/일반 함수 선언은 FunctionObject로 등록해 EvaluateCallExpr(일반 함수 호출 경로)가
// 찾아 쓸 수 있게 한다. 클래스 메서드(init 포함)도 FuncDeclStmtNode를 재사용하지만
// InvokeMethod가 직접 body->Accept(*this)를 호출해서 실행하므로 이 Visit을 거치지 않는다.
void ExecutorUnit::Visit(const FuncDeclStmtNode& node) {
	// children = [파라미터 Identifier..., body(BlockStmt)]. 마지막 원소가 body.
	auto function = std::make_shared<FunctionObject>();
	for (size_t i = 0; i + 1 < node.children.size(); ++i) {
		function->parameters.push_back(node.children[i]->token.lexeme);
	}
	function->body = node.children.back().get();
	scopes.back()[node.token.lexeme] = function;
}

// return문은 값을 계산해 ReturnSignal로 던진다. EvaluateCallExpr/InvokeMethod가 이를 잡아
// 함수/메서드의 반환값으로 사용한다(Checker가 이미 함수/메서드 밖의 return을 막아주므로,
// 이 코드가 함수/메서드 호출 바깥에서 실행되는 일은 없다).
void ExecutorUnit::Visit(const ReturnStmtNode& node) {
	Value_t value = node.children.empty() ? Value_t{ std::monostate{} } : Evaluate(*node.children[0]);
	throw ReturnSignal{ value };
}

void ExecutorUnit::Visit(const NumberLiteralNode& node) { lastValue = node.token.realValue; }
void ExecutorUnit::Visit(const StringLiteralNode& node) { lastValue = node.token.lexeme; }
void ExecutorUnit::Visit(const BoolLiteralNode& node) { lastValue = (node.token.lexeme == "true"); }
void ExecutorUnit::Visit(const IdentifierNode& node) { lastValue = EvaluateIdentifier(node); }
void ExecutorUnit::Visit(const AssignExprNode& node) { lastValue = EvaluateAssignExpr(node); }
void ExecutorUnit::Visit(const BinaryExprNode& node) { lastValue = EvaluateBinaryExpr(node); }
void ExecutorUnit::Visit(const UnaryExprNode& node) { lastValue = EvaluateUnaryExpr(node); }

void ExecutorUnit::Visit(const CallExprNode& node) { lastValue = EvaluateCallExpr(node); }

void ExecutorUnit::Visit(const ArrExprNode& node) { lastValue = EvaluateArrExpr(node); }
void ExecutorUnit::Visit(const IndexExprNode& node) { lastValue = EvaluateIndexExpr(node); }

void ExecutorUnit::Visit(const ThisExprNode& node) {
	if (thisStack.empty()) {
		throw std::runtime_error("클래스 외부에서 'this'를 사용할 수 없습니다 at line " + std::to_string(node.token.line));
	}
	lastValue = thisStack.back();
}

void ExecutorUnit::Visit(const SuperExprNode& node) {
	// Super 단독으로는 this와 같은 인스턴스 값으로 평가된다(필드는 클래스별로 분리되어
	// 저장되지 않으므로 Super.field와 this.field는 동일한 저장소를 가리킨다). Super가
	// 메서드 호출의 대상일 때(Super.move(...))의 "부모부터 찾기" 동작은
	// EvaluateMemberCall이 SuperExprNode 타입을 직접 검사해서 별도로 처리한다.
	if (thisStack.empty()) {
		throw std::runtime_error("클래스 외부에서 'Super'를 사용할 수 없습니다 at line " + std::to_string(node.token.line));
	}
	lastValue = thisStack.back();
}

void ExecutorUnit::Visit(const MemberAccessExprNode& node) { lastValue = EvaluateMemberAccess(node); }

void ExecutorUnit::Visit(const ClassDeclStmtNode& node) {
	ClassInfo info;
	if (!node.parentNameToken.lexeme.empty()) {
		info.parentName = node.parentNameToken.lexeme;
	}
	for (const auto& methodNode : node.children) {
		info.methods[methodNode->token.lexeme] = methodNode.get();
	}
	classes[node.token.lexeme] = std::move(info);
}

// node.children[0]=path, [1]=alias, [2:]=Tokenizer가 이어붙이고 ImportStatementParser가
// ImportEnd 마커까지 소비해 파싱해둔 import 대상 파일의 최상위 문장들.
void ExecutorUnit::Visit(const ImportStmtNode& node) {
	const std::string& aliasName = node.children[1]->token.lexeme;

	std::vector<std::unordered_map<std::string, Value_t>> savedScopes = std::move(scopes);
	scopes.assign(1, {}); // 이 import의 모듈만을 위한 새 스코프 하나

	bool previousSuppress = suppressActionsDuringImport;
	suppressActionsDuringImport = true;
	for (size_t i = 2; i < node.children.size(); ++i) {
		ExecuteStmt(*node.children[i]);
	}
	suppressActionsDuringImport = previousSuppress;

	auto module = std::make_shared<ModuleObject>();
	module->members = std::move(scopes.back());
	scopes = std::move(savedScopes);
	scopes.back()[aliasName] = module;
}

void ExecutorUnit::ExecutePrintStmt(const SyntaxNode& node) {
	const auto& expression = *node.children[0];
	Value_t value = Evaluate(expression);
	if (std::holds_alternative<std::string>(value)) {
		std::cout << std::get<std::string>(value);
	} else if (std::holds_alternative<bool>(value)) {
		std::cout << (std::get<bool>(value) ? "true" : "false");
	} else if (std::holds_alternative<std::monostate>(value)) {
		std::cout << "null";
	} else if (std::holds_alternative<double>(value)) {
		double number = std::get<double>(value);
		if (number == std::floor(number)) {
			std::cout << static_cast<long long>(number);
		} else {
			std::cout << number;
		}
	} else {
		throw std::runtime_error("Unsupported value type for print at line " + std::to_string(node.token.line));
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
	ScopeGuard scopeGuard(*this);
	for (const auto& statement : node.children) {
		ExecuteStmt(*statement);
	}
}

void ExecutorUnit::ExecuteIfStmt(const SyntaxNode& node) {
	const auto& condition = *node.children[0];
	const auto& thenBranch = *node.children[1];
	if (IsTruthy(Evaluate(condition))) {
		ExecuteStmt(thenBranch);
	} else if (node.children.size() > 2) {
		// children[2]는 IfStatementParser가 else/else-if를 붙일 때만 존재한다.
		ExecuteStmt(*node.children[2]);
	}
}

void ExecutorUnit::ExecuteExprStmt(const SyntaxNode& node) {
	const auto& expression = *node.children[0];
	Evaluate(expression);
}

void ExecutorUnit::ExecuteForStmt(const SyntaxNode& node) {
	// init에서 선언된 변수(예: "var i")는 for문 전체(조건/증감/body)에 걸쳐
	// 쓰일 수 있어야 하지만, {} 블록의 지역 변수와 마찬가지로 for문이 끝나면
	// 사라져야 한다. init을 body와 같은 스코프에서 실행해 이 하나의 ScopeGuard로
	// 감싼다(body 자신은 ExecuteBlockStmt가 매 반복마다 별도로 한 겹 더 감싼다).
	ScopeGuard scopeGuard(*this);
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

Value_t ExecutorUnit::EvaluateIdentifier(const IdentifierNode& node) {
	return ResolveVariable(node.token.lexeme, node.scopeDistance, node.token.line);
}

Value_t ExecutorUnit::EvaluateAssignExpr(const SyntaxNode& node) {
	// ExpressionParser는 AssignExpr의 token을 '=' 연산자로, children을
	// [대입 대상, valueExpr] 순서로 둔다. 대입 대상은 일반 변수(Identifier)이거나
	// 배열 인덱스(IndexExpr, 예: arr[0] = 10)일 수 있다.
	const auto& target = *node.children[0];
	const auto& valueExpr = *node.children[1];
	Value_t value = Evaluate(valueExpr);
	if (target.type == NodeType::IndexExpr) {
		ResolveIndexElement(target) = value;
	} else if (target.type == NodeType::MemberAccessExpr) {
		AssignMemberField(static_cast<const MemberAccessExprNode&>(target), value);
	} else if (target.type == NodeType::Identifier) {
		const auto& identifier = static_cast<const IdentifierNode&>(target);
		ResolveVariable(identifier.token.lexeme, identifier.scopeDistance, node.token.line) = value;
	} else {
		// "a + b = 3;"처럼 대입 대상이 변수도, 배열 인덱스도 아닌 임의의
		// 표현식이면(예: BinaryExpr) 여기서 명확히 거부한다. 이 검사가 없으면
		// target을 IdentifierNode로 그냥 static_cast해버려서, 예를 들어
		// BinaryExprNode의 token(연산자 '+')을 변수 이름으로 착각해
		// "Undefined variable '+'" 같은 엉뚱한 메시지가 나갔다.
		throw std::runtime_error("Invalid assignment target at line " + std::to_string(node.token.line));
	}
	return value;
}

Value_t ExecutorUnit::EvaluateArrExpr(const SyntaxNode& node) {
	const auto& sizeExpr = *node.children[0];
	Value_t sizeValue = Evaluate(sizeExpr);
	if (!std::holds_alternative<double>(sizeValue)) {
		throw std::runtime_error("Expected a number for array size at line " + std::to_string(node.token.line));
	}
	double sizeNumber = std::get<double>(sizeValue);
	if (sizeNumber < 0 || sizeNumber != std::floor(sizeNumber)) {
		throw std::runtime_error("Expected a non-negative integer for array size at line " + std::to_string(node.token.line));
	}

	auto array = std::make_shared<ArrayObject>();
	array->elements.assign(static_cast<size_t>(sizeNumber), Value_t{ std::monostate{} });
	return array;
}

Value_t ExecutorUnit::EvaluateIndexExpr(const SyntaxNode& node) {
	return ResolveIndexElement(node);
}

Value_t& ExecutorUnit::ResolveIndexElement(const SyntaxNode& node) {
	const auto& arrayExpr = *node.children[0];
	const auto& indexExpr = *node.children[1];

	Value_t arrayValue = Evaluate(arrayExpr);
	if (!std::holds_alternative<std::shared_ptr<ArrayObject>>(arrayValue)) {
		throw std::runtime_error("Expected an array at line " + std::to_string(node.token.line));
	}
	auto array = std::get<std::shared_ptr<ArrayObject>>(arrayValue);

	Value_t indexValue = Evaluate(indexExpr);
	if (!std::holds_alternative<double>(indexValue)) {
		throw std::runtime_error("Expected a number as array index at line " + std::to_string(node.token.line));
	}
	double indexNumber = std::get<double>(indexValue);

	if (indexNumber < 0 || indexNumber != std::floor(indexNumber) ||
		static_cast<size_t>(indexNumber) >= array->elements.size()) {
		throw std::runtime_error("Array index out of range at line " + std::to_string(node.token.line));
	}

	return array->elements[static_cast<size_t>(indexNumber)];
}

Value_t ExecutorUnit::EvaluateCallExpr(const CallExprNode& node) {
	// r.move(5), this.report(), Super.move(dist), sum.add(1,2)처럼 children[0]이
	// MemberAccessExprNode면 <대상>.<이름>(...) 형태의 메서드/모듈 함수 호출이다.
	if (!node.children.empty() && node.children[0]->type == NodeType::MemberAccessExpr) {
		return EvaluateMemberCall(node);
	}

	// 그 외에는 기존처럼 token.lexeme이 호출 대상 이름이고 children 전체가 인자다.
	// 이름이 등록된 클래스면 인스턴스 생성(Robot(...))으로, 아니면 일반 함수 호출로 처리한다.
	const std::string& name = node.token.lexeme;
	auto classIt = classes.find(name);
	if (classIt != classes.end()) {
		return InstantiateClass(name, node);
	}

	// 일반 함수 호출: CallExprNode의 token은 callee 식별자 토큰이고, scopeDistance가
	// 없으므로 항상 동적(선형) 탐색으로 함수 값을 찾는다.
	Value_t calleeValue = ResolveVariable(name, -1, node.token.line);
	if (!std::holds_alternative<std::shared_ptr<FunctionObject>>(calleeValue)) {
		throw std::runtime_error("'" + name + "' is not callable at line " + std::to_string(node.token.line));
	}
	auto function = std::get<std::shared_ptr<FunctionObject>>(calleeValue);

	std::vector<Value_t> args;
	args.reserve(node.children.size());
	for (const auto& argExpr : node.children) {
		args.push_back(Evaluate(*argExpr));
	}

	return CallFunction(function, args, node.token.line);
}

Value_t ExecutorUnit::CallFunction(std::shared_ptr<FunctionObject> function, const std::vector<Value_t>& args, int line) {
	if (args.size() != function->parameters.size()) {
		throw std::runtime_error("Expected " + std::to_string(function->parameters.size()) +
			" argument(s) but got " + std::to_string(args.size()) +
			" at line " + std::to_string(line));
	}

	// 함수 본문은 caller의 지역 스코프를 보지 못해야 하므로, global(scopes[0])만
	// 남기고 나머지 프레임은 잠시 떼어둔 뒤 파라미터 스코프 하나만 새로 쌓는다.
	FunctionCallFrameGuard frameGuard(*this);
	for (size_t i = 0; i < function->parameters.size(); ++i) {
		scopes.back()[function->parameters[i]] = args[i];
	}

	return ExecuteFunctionBody(*function->body);
}

Value_t ExecutorUnit::InvokeModuleFunction(std::shared_ptr<ModuleObject> module, const std::string& functionName, const CallExprNode& node) {
	auto memberIt = module->members.find(functionName);
	if (memberIt == module->members.end() || !std::holds_alternative<std::shared_ptr<FunctionObject>>(memberIt->second)) {
		throw std::runtime_error("존재하지 않는 함수 '" + functionName + "'을(를) 호출했습니다 at line " + std::to_string(node.token.line));
	}

	std::vector<Value_t> args;
	for (size_t i = 1; i < node.children.size(); ++i) {
		args.push_back(Evaluate(*node.children[i]));
	}

	return CallFunction(std::get<std::shared_ptr<FunctionObject>>(memberIt->second), args, node.token.line);
}

Value_t ExecutorUnit::EvaluateMemberCall(const CallExprNode& node) {
	const auto& memberAccess = static_cast<const MemberAccessExprNode&>(*node.children[0]);
	const std::string& memberName = memberAccess.token.lexeme;
	const auto& targetExpr = *memberAccess.children[0];

	std::shared_ptr<InstanceObject> instance;
	std::string searchStartClass;

	// Super.move(...)는 인스턴스의 실제(가장 파생된) 클래스가 아니라, "현재 실행 중인
	// 메서드가 정의된 클래스"의 부모부터 찾아야 한다(그래야 오버라이딩한 자기 자신을
	// 다시 호출하는 무한 재귀에 빠지지 않는다).
	if (targetExpr.type == NodeType::SuperExpr) {
		if (currentClassNameStack.empty()) {
			throw std::runtime_error("클래스 외부에서 'Super'를 사용할 수 없습니다 at line " + std::to_string(node.token.line));
		}
		const ClassInfo& currentClassInfo = classes.at(currentClassNameStack.back());
		if (!currentClassInfo.parentName) {
			throw std::runtime_error("부모 클래스가 없는 클래스에서 'Super'를 사용할 수 없습니다 at line " + std::to_string(node.token.line));
		}
		instance = thisStack.back();
		searchStartClass = *currentClassInfo.parentName;
	} else {
		Value_t targetValue = Evaluate(targetExpr);
		if (std::holds_alternative<std::shared_ptr<ModuleObject>>(targetValue)) {
			return InvokeModuleFunction(std::get<std::shared_ptr<ModuleObject>>(targetValue), memberName, node);
		}
		if (!std::holds_alternative<std::shared_ptr<InstanceObject>>(targetValue)) {
			throw std::runtime_error("인스턴스가 아닌 값의 메서드 '" + memberName + "'을(를) 호출할 수 없습니다 at line " + std::to_string(node.token.line));
		}
		instance = std::get<std::shared_ptr<InstanceObject>>(targetValue);
		searchStartClass = instance->className;
	}

	ResolvedMethod resolved = FindMethod(searchStartClass, memberName);
	if (!resolved.method) {
		throw std::runtime_error("존재하지 않는 메서드 '" + memberName + "'을(를) 호출했습니다 at line " + std::to_string(node.token.line));
	}

	std::vector<Value_t> args;
	for (size_t i = 1; i < node.children.size(); ++i) {
		args.push_back(Evaluate(*node.children[i]));
	}

	return InvokeMethod(*resolved.method, resolved.owningClassName, instance, args);
}

Value_t ExecutorUnit::EvaluateMemberAccess(const MemberAccessExprNode& node) {
	Value_t targetValue = Evaluate(*node.children[0]);

	// sum.PI처럼 모듈의 전역 변수를 읽는 경우.
	if (std::holds_alternative<std::shared_ptr<ModuleObject>>(targetValue)) {
		auto module = std::get<std::shared_ptr<ModuleObject>>(targetValue);
		auto memberIt = module->members.find(node.token.lexeme);
		if (memberIt == module->members.end()) {
			throw std::runtime_error("존재하지 않는 이름 '" + node.token.lexeme + "'에 접근했습니다 at line " + std::to_string(node.token.line));
		}
		return memberIt->second;
	}

	if (!std::holds_alternative<std::shared_ptr<InstanceObject>>(targetValue)) {
		throw std::runtime_error("인스턴스가 아닌 값에 멤버 '" + node.token.lexeme + "'로 접근할 수 없습니다 at line " + std::to_string(node.token.line));
	}
	auto instance = std::get<std::shared_ptr<InstanceObject>>(targetValue);

	auto fieldIt = instance->fields.find(node.token.lexeme);
	if (fieldIt == instance->fields.end()) {
		throw std::runtime_error("존재하지 않는 필드 '" + node.token.lexeme + "'에 접근했습니다 at line " + std::to_string(node.token.line));
	}
	return fieldIt->second;
}

void ExecutorUnit::AssignMemberField(const MemberAccessExprNode& node, const Value_t& value) {
	Value_t targetValue = Evaluate(*node.children[0]);
	if (!std::holds_alternative<std::shared_ptr<InstanceObject>>(targetValue)) {
		throw std::runtime_error("인스턴스가 아닌 값에 멤버 '" + node.token.lexeme + "'로 접근할 수 없습니다 at line " + std::to_string(node.token.line));
	}
	auto instance = std::get<std::shared_ptr<InstanceObject>>(targetValue);
	instance->fields[node.token.lexeme] = value; // 없는 필드면 새로 생성(슬라이드 "필드 쓰기" 명세)
}

Value_t ExecutorUnit::InstantiateClass(const std::string& className, const SyntaxNode& callNode) {
	auto instance = std::make_shared<InstanceObject>();
	instance->className = className;

	ResolvedMethod initMethod = FindMethod(className, "init");
	if (initMethod.method) {
		std::vector<Value_t> args;
		for (const auto& argExpr : callNode.children) {
			args.push_back(Evaluate(*argExpr));
		}
		InvokeMethod(*initMethod.method, initMethod.owningClassName, instance, args);
	}
	return instance;
}

Value_t ExecutorUnit::InvokeMethod(const SyntaxNode& methodNode, const std::string& owningClassName,
	std::shared_ptr<InstanceObject> instance, const std::vector<Value_t>& args) {
	// FuncDeclStmtNode와 동일한 구조를 재사용: children = [파라미터(Identifier)..., body(BlockStmt)].
	const SyntaxNode* body = methodNode.children.back().get();

	ScopeGuard scopeGuard(*this);
	for (size_t i = 0; i + 1 < methodNode.children.size(); ++i) {
		const std::string& paramName = methodNode.children[i]->token.lexeme;
		scopes.back()[paramName] = (i < args.size()) ? args[i] : Value_t{ std::monostate{} };
	}

	MethodContextGuard methodGuard(*this, instance, owningClassName);
	return ExecuteFunctionBody(*body);
}

ExecutorUnit::ResolvedMethod ExecutorUnit::FindMethod(const std::string& startClassName, const std::string& methodName) const {
	std::string current = startClassName;
	while (true) {
		auto classIt = classes.find(current);
		if (classIt == classes.end()) {
			return {};
		}
		auto methodIt = classIt->second.methods.find(methodName);
		if (methodIt != classIt->second.methods.end()) {
			return { methodIt->second, current };
		}
		if (!classIt->second.parentName) {
			return {};
		}
		current = *classIt->second.parentName;
	}
}

bool ExecutorUnit::IsInstanceOf(const std::string& actualClassName, const std::string& targetClassName) const {
	std::string current = actualClassName;
	while (true) {
		if (current == targetClassName) {
			return true;
		}
		auto classIt = classes.find(current);
		if (classIt == classes.end() || !classIt->second.parentName) {
			return false;
		}
		current = *classIt->second.parentName;
	}
}

Value_t ExecutorUnit::EvaluateBinaryExpr(const BinaryExprNode& node) {
	if (node.isConstantFolded) {
		return node.foldedValue;
	}

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

	// instanceof는 우변을 값으로 평가하지 않는다 — 클래스 이름을 직접 가리키는
	// Identifier 토큰이기 때문이다(w instanceof Robot에서 Robot은 변수가 아니다).
	if (node.token.type == TokenType::KwInstanceof) {
		Value_t leftValue = Evaluate(leftExpr);
		const std::string& targetClassName = rightExpr.token.lexeme;
		if (!std::holds_alternative<std::shared_ptr<InstanceObject>>(leftValue)) {
			return false;
		}
		return IsInstanceOf(std::get<std::shared_ptr<InstanceObject>>(leftValue)->className, targetClassName);
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

Value_t ExecutorUnit::EvaluateUnaryExpr(const UnaryExprNode& node) {
	if (node.isConstantFolded) {
		return node.foldedValue;
	}

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
