#pragma once
#include <memory>
#include <vector>
#include "Token.h"

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

	// statement
	VarDeclStmt,
	ExprStmt,
	IfStmt,
	WhileStmt,
	BlockStmt,
	FuncDeclStmt,
	ReturnStmt,

	Program
};

class SyntaxNode {
public:
	NodeType type;
	Token token;
	std::vector<std::unique_ptr<SyntaxNode>> children;
};
