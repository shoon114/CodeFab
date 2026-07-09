#pragma once

class SyntaxNode;
class NumberLiteralNode;
class StringLiteralNode;
class BoolLiteralNode;
class IdentifierNode;
class BinaryExprNode;
class UnaryExprNode;
class AssignExprNode;
class CallExprNode;
class ArrExprNode;
class IndexExprNode;
class ThisExprNode;
class SuperExprNode;
class MemberAccessExprNode;
class VarDeclareStatementNode;
class ExprStmtNode;
class PrintStmtNode;
class IfStmtNode;
class ForStmtNode;
class BlockStmtNode;
class FuncDeclStmtNode;
class ReturnStmtNode;
class ClassDeclStmtNode;
class ProgramNode;

// SyntaxNode 계층에 대한 Visitor 인터페이스.
// 각 Visit()의 기본 구현(NodeVisitor.cpp)은 Traverse()로 자식 노드를 그대로
// 훑는 것이므로, 구체 Visitor는 관심 있는 노드 타입만 override하면 된다.
// 순회를 멈추고 싶은 override(예: 스코프를 직접 관리해야 하는 경우)는
// Traverse()를 호출하지 않으면 된다.
//
// 기본 구현은 여기(헤더)가 아니라 NodeVisitor.cpp에 있다: 각 XxxNode가
// SyntaxNode를 상속한다는 사실은 전방 선언만으로는 알 수 없어서(불완전 타입),
// Traverse(node)를 호출하려면 SyntaxNode.h가 완전히 include된 시점 이후에
// 정의해야 한다.
class NodeVisitor {
public:
	virtual ~NodeVisitor() = default;

	virtual void Visit(const NumberLiteralNode& node);
	virtual void Visit(const StringLiteralNode& node);
	virtual void Visit(const BoolLiteralNode& node);
	virtual void Visit(const IdentifierNode& node);
	virtual void Visit(const BinaryExprNode& node);
	virtual void Visit(const UnaryExprNode& node);
	virtual void Visit(const AssignExprNode& node);
	virtual void Visit(const CallExprNode& node);
	virtual void Visit(const ArrExprNode& node);
	virtual void Visit(const IndexExprNode& node);
	virtual void Visit(const ThisExprNode& node);
	virtual void Visit(const SuperExprNode& node);
	virtual void Visit(const MemberAccessExprNode& node);
	virtual void Visit(const VarDeclareStatementNode& node);
	virtual void Visit(const ExprStmtNode& node);
	virtual void Visit(const PrintStmtNode& node);
	virtual void Visit(const IfStmtNode& node);
	virtual void Visit(const ForStmtNode& node);
	virtual void Visit(const BlockStmtNode& node);
	virtual void Visit(const FuncDeclStmtNode& node);
	virtual void Visit(const ReturnStmtNode& node);
	virtual void Visit(const ClassDeclStmtNode& node);
	virtual void Visit(const ProgramNode& node);

protected:
	// 자식 노드들을 순서대로 Accept()하여 재귀적으로 순회한다.
	void Traverse(const SyntaxNode& node);
};
