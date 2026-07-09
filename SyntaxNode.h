#pragma once
#include <memory>
#include <vector>
#include "Token.h"
#include "NodeVisitor.h"
#include "Value.h"

enum class NodeType {
	// expression
	NumberLiteral,
	StringLiteral,
	BoolLiteral,
	Identifier,
	BinaryExpr,
	UnaryExpr,
	AssignExpr,
	CallExpr,
	ArrExpr,
	IndexExpr,
	ThisExpr,
	SuperExpr,
	MemberAccessExpr,

	// statement
	VarDeclareStatement,
	ExprStmt,
	PrintStmt,
	IfStmt,
	ForStmt,
	BlockStmt,
	FuncDeclStmt,
	ReturnStmt,
	ClassDeclStmt,
	ImportStmt,

	Program
};

// AST 노드의 추상 베이스. Visitor 패턴의 더블 디스패치를 위해 Accept()를
// 각 구체 타입(아래)이 override하여 자기 자신을 정확한 타입으로 NodeVisitor에 넘긴다.
// type 필드는 각 구체 클래스의 생성자가 채우므로, 노드를 생성할 때 별도로
// 설정할 필요는 없다(구체 타입 자체가 이미 타입 정보를 담고 있음).
class SyntaxNode {
public:
	NodeType type;
	Token token;
	std::vector<std::unique_ptr<SyntaxNode>> children;

	virtual ~SyntaxNode() = default;
	virtual void Accept(NodeVisitor& visitor) const = 0;
};

class NumberLiteralNode : public SyntaxNode {
public:
	NumberLiteralNode() { type = NodeType::NumberLiteral; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

class StringLiteralNode : public SyntaxNode {
public:
	StringLiteralNode() { type = NodeType::StringLiteral; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

class BoolLiteralNode : public SyntaxNode {
public:
	BoolLiteralNode() { type = NodeType::BoolLiteral; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

class IdentifierNode : public SyntaxNode {
public:
	IdentifierNode() { type = NodeType::Identifier; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }

	// CheckerUnit이 정적 바인딩 단계에서 채워두는, 선언된 스코프까지의 거리(홉 수).
	// -1이면 미해결(체커가 못 찾음) 상태이며, ExecutorUnit은 이 경우 기존 동적 탐색으로 폴백한다.
	mutable int scopeDistance = -1;
};

class BinaryExprNode : public SyntaxNode {
public:
	BinaryExprNode() { type = NodeType::BinaryExpr; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }

	// CheckerUnit이 상수 폴딩 단계에서 채워두는, 컴파일 타임에 미리 계산된 값.
	mutable bool isConstantFolded = false;
	mutable Value_t foldedValue{};
};

class UnaryExprNode : public SyntaxNode {
public:
	UnaryExprNode() { type = NodeType::UnaryExpr; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }

	mutable bool isConstantFolded = false;
	mutable Value_t foldedValue{};
};

class AssignExprNode : public SyntaxNode {
public:
	AssignExprNode() { type = NodeType::AssignExpr; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

// token은 callee 식별자 토큰(호출 대상 이름).
class CallExprNode : public SyntaxNode {
public:
	CallExprNode() { type = NodeType::CallExpr; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

// Array(size) 배열 생성 전용 노드. children = [size식]. token은 'Array' 식별자 토큰.
// 일반 함수 호출(CallExprNode)과 분리해두면 Checker/Executor가 사용자 정의 함수
// 호출과 배열 생성을 타입 검사만으로(문자열 이름 비교 없이) 구분할 수 있다.
class ArrExprNode : public SyntaxNode {
public:
	ArrExprNode() { type = NodeType::ArrExpr; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

// children = [array식, index식]. token은 여는 '[' 토큰(위치 정보용).
class IndexExprNode : public SyntaxNode {
public:
	IndexExprNode() { type = NodeType::IndexExpr; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

// 클래스 메서드 내부에서 자기 인스턴스를 가리키는 'this'. children 없음.
class ThisExprNode : public SyntaxNode {
public:
	ThisExprNode() { type = NodeType::ThisExpr; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

// 클래스 메서드 내부에서 부모 클래스를 가리키는 'Super'. children 없음.
class SuperExprNode : public SyntaxNode {
public:
	SuperExprNode() { type = NodeType::SuperExpr; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

// <대상>.<멤버> 형태의 필드/메서드 접근. children = [대상식]. token은 멤버 이름 토큰
// (예: 'name', 'move'). r.speed, this.name, Super.move 모두 이 노드로 표현한다.
class MemberAccessExprNode : public SyntaxNode {
public:
	MemberAccessExprNode() { type = NodeType::MemberAccessExpr; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

class VarDeclareStatementNode : public SyntaxNode {
public:
	VarDeclareStatementNode() { type = NodeType::VarDeclareStatement; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

class ExprStmtNode : public SyntaxNode {
public:
	ExprStmtNode() { type = NodeType::ExprStmt; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

class PrintStmtNode : public SyntaxNode {
public:
	PrintStmtNode() { type = NodeType::PrintStmt; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

class IfStmtNode : public SyntaxNode {
public:
	IfStmtNode() { type = NodeType::IfStmt; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

class ForStmtNode : public SyntaxNode {
public:
	ForStmtNode() { type = NodeType::ForStmt; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

class BlockStmtNode : public SyntaxNode {
public:
	BlockStmtNode() { type = NodeType::BlockStmt; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

class FuncDeclStmtNode : public SyntaxNode {
public:
	FuncDeclStmtNode() { type = NodeType::FuncDeclStmt; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

class ReturnStmtNode : public SyntaxNode {
public:
	ReturnStmtNode() { type = NodeType::ReturnStmt; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};

// Class <이름> [: <부모이름>] { 메서드... }
// token = 클래스 이름 토큰. parentNameToken은 상속 대상 이름 토큰(lexeme이 비어있으면
// 부모 없음을 뜻한다). children = 메서드 선언들 — FuncDeclStmtNode를 그대로 재사용하며
// (파라미터... , body 순서로 children을 갖는 기존 구조 그대로), 이름이 "init"인 메서드는
// 생성자로 취급한다.
class ClassDeclStmtNode : public SyntaxNode {
public:
	ClassDeclStmtNode() { type = NodeType::ClassDeclStmt; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }

	Token parentNameToken;
};

// import "파일주소" alias 별칭명;
// token = import 키워드 토큰, pathToken = 문자열 literal, aliasToken = 별칭 식별자.
class ImportStmtNode : public SyntaxNode {
public:
	ImportStmtNode() { type = NodeType::ImportStmt; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }

	Token pathToken;
	Token aliasToken;
};

class ProgramNode : public SyntaxNode {
public:
	ProgramNode() { type = NodeType::Program; }
	void Accept(NodeVisitor& visitor) const override { visitor.Visit(*this); }
};
