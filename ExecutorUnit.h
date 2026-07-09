#pragma once
#include "NodeVisitor.h"
#include "SyntaxNode.h"
#include "Value.h"
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class ExecutorUnit : public NodeVisitor {
public:
	void Execute(const SyntaxNode& tree);

	// 문(statement) 노드: 실행 후 값을 반환하지 않는다.
	void Visit(const PrintStmtNode& node) override;
	void Visit(const VarDeclareStatementNode& node) override;
	void Visit(const BlockStmtNode& node) override;
	void Visit(const IfStmtNode& node) override;
	void Visit(const ExprStmtNode& node) override;
	void Visit(const ForStmtNode& node) override;
	void Visit(const FuncDeclStmtNode& node) override;
	void Visit(const ReturnStmtNode& node) override;
	void Visit(const ImportStmtNode& node) override;

	// 식(expression) 노드: 결과를 lastValue에 남기고, Evaluate()가 이를 읽어 반환한다.
	void Visit(const NumberLiteralNode& node) override;
	void Visit(const StringLiteralNode& node) override;
	void Visit(const BoolLiteralNode& node) override;
	void Visit(const IdentifierNode& node) override;
	void Visit(const AssignExprNode& node) override;
	void Visit(const BinaryExprNode& node) override;
	void Visit(const UnaryExprNode& node) override;
	void Visit(const CallExprNode& node) override;
	void Visit(const ArrExprNode& node) override;
	void Visit(const IndexExprNode& node) override;
	void Visit(const ThisExprNode& node) override;
	void Visit(const SuperExprNode& node) override;
	void Visit(const MemberAccessExprNode& node) override;
	void Visit(const ClassDeclStmtNode& node) override;

private:
	class ScopeGuard;

	// 클래스 하나의 런타임 정보: 부모 이름(없으면 nullopt) + 메서드 이름 -> AST(FuncDeclStmtNode) 포인터.
	// 트리는 Execute() 호출 동안 계속 살아있으므로 raw pointer로 충분하다(FunctionObject::body와 동일한 관례).
	struct ClassInfo {
		std::optional<std::string> parentName;
		std::unordered_map<std::string, const SyntaxNode*> methods;
	};

	// FindMethod의 결과: 실제로 메서드를 찾았는지, 그리고 그 메서드가 어느 클래스에
	// "정의"되어 있는지(Super 호출 시 이 값을 기준으로 그 부모부터 다시 찾아야 한다).
	struct ResolvedMethod {
		const SyntaxNode* method = nullptr;
		std::string owningClassName;
	};
	void ExecuteStmt(const SyntaxNode& node);
	Value_t Evaluate(const SyntaxNode& node);

	// Visit(Stmt)가 위임하는 문(statement)별 실행 메서드
	void ExecutePrintStmt(const SyntaxNode& node);
	void ExecuteVarDeclareStatement(const SyntaxNode& node);
	void ExecuteBlockStmt(const SyntaxNode& node);
	void ExecuteIfStmt(const SyntaxNode& node);
	void ExecuteExprStmt(const SyntaxNode& node);
	void ExecuteForStmt(const SyntaxNode& node);
	void ExecuteImportStmt(const ImportStmtNode& node);

	// Visit(Expr)가 위임하는 식(expression)별 평가 메서드
	Value_t EvaluateIdentifier(const IdentifierNode& node);
	Value_t EvaluateAssignExpr(const SyntaxNode& node);
	Value_t EvaluateBinaryExpr(const BinaryExprNode& node);
	Value_t EvaluateUnaryExpr(const UnaryExprNode& node);
	Value_t EvaluateArrExpr(const SyntaxNode& node);
	Value_t EvaluateIndexExpr(const SyntaxNode& node);
	Value_t EvaluateCallExpr(const CallExprNode& node);
	Value_t InvokeFunction(std::shared_ptr<FunctionObject> function, const std::vector<Value_t>& args);
	Value_t EvaluateMethodCall(const CallExprNode& node);
	Value_t EvaluateMemberAccess(const MemberAccessExprNode& node);

	void EnterScope();
	void ExitScope();
	// distance는 CheckerUnit이 미리 계산해둔, 선언 스코프까지의 홉 수(정적 바인딩).
	// -1이면 미해결 상태이므로 기존 동적(선형) 탐색으로 폴백한다.
	Value_t& ResolveVariable(const std::string& name, int distance, int line);
	// arr[index] 형태의 노드(children = [array식, index식])를 평가해 배열/인덱스를
	// 검증한 뒤 해당 칸의 참조를 돌려준다. 읽기(EvaluateIndexExpr)와 쓰기
	// (EvaluateAssignExpr)가 이 함수 하나를 공유해 검증 로직이 중복되지 않게 한다.
	Value_t& ResolveIndexElement(const SyntaxNode& node);
	double AsNumber(const Value_t& value, int line);
	bool IsTruthy(const Value_t& value);
	void EnsureNonZeroDivisor(double divisor, int line);

	// r.field = value; / this.field = value; 등 MemberAccessExprNode를 대입 대상으로
	// 쓸 때 호출된다. 필드가 없으면 새로 생성한다(슬라이드 "필드 쓰기" 명세).
	void AssignMemberField(const MemberAccessExprNode& node, const Value_t& value);
	// Robot(...) / Robot("AndOr", 10)처럼 클래스 이름을 호출해 인스턴스를 생성한다.
	// init 메서드가 있으면 callNode의 인자로 즉시 호출한다.
	Value_t InstantiateClass(const std::string& className, const SyntaxNode& callNode);
	// 메서드(또는 init) 본문 하나를 this/파라미터를 바인딩한 새 스코프에서 실행한다.
	// return문은 ReturnSignal 예외로 구현되어 있어 이 함수가 잡아 반환값으로 변환한다.
	Value_t InvokeMethod(const SyntaxNode& methodNode, const std::string& owningClassName,
		std::shared_ptr<InstanceObject> instance, const std::vector<Value_t>& args);
	// startClassName부터 부모 방향으로 올라가며 methodName을 찾는다. Super 호출은
	// startClassName으로 "현재 메서드가 정의된 클래스의 부모"를 넘겨 받아 재사용한다.
	ResolvedMethod FindMethod(const std::string& startClassName, const std::string& methodName) const;
	// actualClassName이 targetClassName 자신이거나, 그 조상 클래스 중 하나인지 검사한다.
	bool IsInstanceOf(const std::string& actualClassName, const std::string& targetClassName) const;
	std::shared_ptr<ModuleObject> LoadModule(const std::string& path, int line);
	std::string ResolveImportPath(const std::string& path) const;
	void EnsureImportAllowed(const std::string& resolvedPath, const std::string& aliasName, int line) const;
	std::shared_ptr<FunctionObject> MakeFunctionObject(const SyntaxNode& functionNode,
		std::shared_ptr<std::unordered_map<std::string, Value_t>> closure = nullptr) const;

	// scopes[0]이 Global 스코프, 이후 요소는 BlockStmt 진입마다 추가되는 Local 스코프.
	std::vector<std::unordered_map<std::string, Value_t>> scopes;
	std::vector<std::unordered_set<std::string>> importedPathScopes;
	std::vector<std::string> importStack;
	std::vector<std::unique_ptr<SyntaxNode>> importedTrees;

	// 클래스 이름 -> 런타임 정보. ClassDeclStmtNode를 실행(Visit)할 때 등록된다.
	std::unordered_map<std::string, ClassInfo> classes;
	// 현재 실행 중인 메서드들의 this 인스턴스 스택(메서드 호출마다 push/pop).
	std::vector<std::shared_ptr<InstanceObject>> thisStack;
	// 현재 실행 중인 메서드가 "정의된" 클래스 이름 스택(인스턴스의 실제 클래스가 아니라
	// lexical하게 그 메서드를 담고 있는 클래스 — Super 호출의 시작점을 정하는 데 쓰인다).
	std::vector<std::string> currentClassNameStack;
	int loopDepth = 0;

	// Evaluate()가 Accept()를 통해 식 노드를 방문한 뒤 결과를 꺼내가는 임시 저장소.
	Value_t lastValue;
};
