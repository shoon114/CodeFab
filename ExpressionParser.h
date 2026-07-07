#pragma once
#include "IExpressionParser.h"

// Pratt parser (operator-precedence parsing): a single precedence-climbing
// loop driven by a per-token binding-power table, instead of one function
// per precedence level.
class ExpressionParser : public IExpressionParser {
public:
	std::unique_ptr<SyntaxNode> Parse(const TokenList& tokenList, size_t& pos) override;

private:
	// Parses an expression whose infix operators all bind at least as tightly as minPrecedence.
	std::unique_ptr<SyntaxNode> ParseExpression(const TokenList& tokenList, size_t& pos, int minPrecedence);

	// Prefix position: unary operators, literals, identifiers, parenthesized expressions, calls.
	std::unique_ptr<SyntaxNode> ParsePrefix(const TokenList& tokenList, size_t& pos);
	std::unique_ptr<SyntaxNode> ParsePrimary(const TokenList& tokenList, size_t& pos);
	std::unique_ptr<SyntaxNode> ParseCallArguments(const TokenList& tokenList, size_t& pos, std::unique_ptr<SyntaxNode> callee);

	static std::unique_ptr<SyntaxNode> MakeBinary(NodeType type, Token op, std::unique_ptr<SyntaxNode> left, std::unique_ptr<SyntaxNode> right);
};
