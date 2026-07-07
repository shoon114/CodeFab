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
}

std::unique_ptr<SyntaxNode> ForStmtParser::Parse(const TokenList& tokenList, size_t& pos) {
	const Token& forToken = tokenList[pos++]; // 'for'
	pos++; // '('

	// init resolves whatever is registered for its leading token (e.g.
	// VarDeclareParser for 'var') and consumes up to and including its own
	// trailing ';'.
	auto initNode = ResolveAndParse(tokenList, pos, "an initializer statement in 'for'");

	auto condNode = ResolveAndParse(tokenList, pos, "a condition expression in 'for'");
	pos++; // ';'

	auto updateNode = ResolveAndParse(tokenList, pos, "an update expression in 'for'");
	pos++; // ')'

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
