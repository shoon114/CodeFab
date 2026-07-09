#include "NodeVisitor.h"
#include "SyntaxNode.h"

void NodeVisitor::Traverse(const SyntaxNode& node) {
	for (const auto& child : node.children) {
		child->Accept(*this);
	}
}

// 기본 구현: 관심 없는 노드 타입은 그냥 자식으로 내려간다.
void NodeVisitor::Visit(const NumberLiteralNode& node) { Traverse(node); }
void NodeVisitor::Visit(const StringLiteralNode& node) { Traverse(node); }
void NodeVisitor::Visit(const BoolLiteralNode& node) { Traverse(node); }
void NodeVisitor::Visit(const IdentifierNode& node) { Traverse(node); }
void NodeVisitor::Visit(const BinaryExprNode& node) { Traverse(node); }
void NodeVisitor::Visit(const UnaryExprNode& node) { Traverse(node); }
void NodeVisitor::Visit(const AssignExprNode& node) { Traverse(node); }
void NodeVisitor::Visit(const CallExprNode& node) { Traverse(node); }
void NodeVisitor::Visit(const ArrExprNode& node) { Traverse(node); }
void NodeVisitor::Visit(const IndexExprNode& node) { Traverse(node); }
void NodeVisitor::Visit(const ThisExprNode& node) { Traverse(node); }
void NodeVisitor::Visit(const SuperExprNode& node) { Traverse(node); }
void NodeVisitor::Visit(const MemberAccessExprNode& node) { Traverse(node); }
void NodeVisitor::Visit(const VarDeclareStatementNode& node) { Traverse(node); }
void NodeVisitor::Visit(const ExprStmtNode& node) { Traverse(node); }
void NodeVisitor::Visit(const PrintStmtNode& node) { Traverse(node); }
void NodeVisitor::Visit(const IfStmtNode& node) { Traverse(node); }
void NodeVisitor::Visit(const ForStmtNode& node) { Traverse(node); }
void NodeVisitor::Visit(const BlockStmtNode& node) { Traverse(node); }
void NodeVisitor::Visit(const FuncDeclStmtNode& node) { Traverse(node); }
void NodeVisitor::Visit(const ReturnStmtNode& node) { Traverse(node); }
void NodeVisitor::Visit(const ClassDeclStmtNode& node) { Traverse(node); }
void NodeVisitor::Visit(const ProgramNode& node) { Traverse(node); }
