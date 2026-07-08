#include "ForStmtParser.h"
#include "StatementParserRegistry.h"
#include <stdexcept>

namespace {
	StatementParserRegistrar<ForStmtParser> registrar(TokenType::KwFor);

	std::unique_ptr<SyntaxNode> ResolveAndParse(const TokenList& tokenList, size_t& pos, const char* what) {
		std::shared_ptr<IStatementParser> parser = StatementParserRegistry::Instance().Resolve(tokenList[pos].type);
		if (parser == nullptr) {
			throw std::runtime_error(std::string("Expected ") + what + " at line " + std::to_string(tokenList[pos].line));
		}
		return parser->Parse(tokenList, pos);
	}

	void ExpectToken(const TokenList& tokenList, size_t& pos, TokenType expected, const char* what) {
		if (pos >= tokenList.size() || tokenList[pos].type != expected) {
			int line = pos < tokenList.size() ? tokenList[pos].line : tokenList.back().line;
			throw std::runtime_error(std::string("Expected ") + what + " at line " + std::to_string(line));
		}
		pos++;
	}
}

std::unique_ptr<SyntaxNode> ForStmtParser::Parse(const TokenList& tokenList, size_t& pos) {
	const Token& forToken = tokenList[pos++]; // 'for'

	ExpectToken(tokenList, pos, TokenType::LParen, "'(' after 'for'");

	// init resolves whatever is registered for its leading token (e.g.
	// VarDeclareParser for 'var') and consumes up to and including its own
	// trailing ';'.
	auto initNode = ResolveAndParse(tokenList, pos, "an initializer statement in 'for'");

	auto condNode = ResolveAndParse(tokenList, pos, "a condition expression in 'for'");
	ExpectToken(tokenList, pos, TokenType::Semicolon, "';' after for-loop condition");

	auto updateNode = ResolveAndParse(tokenList, pos, "an update expression in 'for'");
	ExpectToken(tokenList, pos, TokenType::RParen, "')' after for-loop update expression");

	pos++; // '{'
	pos++; // '}'  (empty body for now)

	auto bodyNode = std::make_unique<BlockStmtNode>();

	auto forNode = std::make_unique<ForStmtNode>();
	forNode->token = forToken;
	forNode->children.push_back(std::move(initNode));
	forNode->children.push_back(std::move(condNode));
	forNode->children.push_back(std::move(updateNode));
	forNode->children.push_back(std::move(bodyNode));

	return forNode;
}
