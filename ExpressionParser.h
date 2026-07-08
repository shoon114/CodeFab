#pragma once
#include "IStatementParser.h"

// Pratt parser (operator-precedence parsing): a single precedence-climbing
// loop driven by a per-token binding-power table, instead of one function
// per precedence level.
class ExpressionParser : public IStatementParser {
public:
	std::unique_ptr<SyntaxNode> Parse(const TokenList& tokenList, size_t& pos) override;

private:
	// Parses an expression whose infix operators all bind at least as tightly as minPrecedence.
	std::unique_ptr<SyntaxNode> ParseExpression(const TokenList& tokenList, size_t& pos, int minPrecedence);

	// Prefix position: unary operators, literals, identifiers, parenthesized expressions, calls.
	std::unique_ptr<SyntaxNode> ParsePrefix(const TokenList& tokenList, size_t& pos);

	// ParseAtom이 만든 식에 이어지는 postfix(함수 호출 '(...)', 인덱싱 '[...]')를
	// 계속 붙여나간다 (예: arr[0], foo(1, 2), arr[i][j]).
	std::unique_ptr<SyntaxNode> ParsePrimary(const TokenList& tokenList, size_t& pos);
	std::unique_ptr<SyntaxNode> ParseAtom(const TokenList& tokenList, size_t& pos);
	std::unique_ptr<SyntaxNode> ParseCallExpr(const TokenList& tokenList, size_t& pos, std::unique_ptr<SyntaxNode> callee);
	std::unique_ptr<SyntaxNode> ParseArrExpr(const TokenList& tokenList, size_t& pos, Token calleeToken);
	std::unique_ptr<SyntaxNode> ParseIndexExpr(const TokenList& tokenList, size_t& pos, std::unique_ptr<SyntaxNode> array);

	static std::unique_ptr<SyntaxNode> MakeBinary(NodeType type, Token op, std::unique_ptr<SyntaxNode> left, std::unique_ptr<SyntaxNode> right);
};
