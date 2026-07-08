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

	// AssemblerUnit/BlockParser가 최상위 statement 뒤에서 하는 것과 동일하게,
	// body로 쓰인 bare 표현식 문장(ExpressionParser)은 자신의 trailing ';'를
	// 스스로 소비하지 않으므로 여기서 남은 ';'를 소비해준다. '{' 로 시작하는
	// 블록 body(BlockParser)처럼 이미 다 소비된 경우엔 아무 일도 하지 않는다.
	void ConsumeOptionalTrailingSemicolon(const TokenList& tokenList, size_t& pos) {
		if (pos < tokenList.size() && tokenList[pos].type == TokenType::Semicolon) {
			pos++;
		}
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

	// body는 '{' 블록일 수도(BlockParser), '{}' 없는 단일 statement(예: print,
	// var, bare 대입문 등)일 수도 있다. init/condition/update와 마찬가지로
	// 어떤 형태인지 ForStmtParser가 직접 알 필요 없이 registry에 위임한다.
	auto bodyNode = ResolveAndParse(tokenList, pos, "a statement to start for-loop body");
	ConsumeOptionalTrailingSemicolon(tokenList, pos);

	auto forNode = std::make_unique<ForStmtNode>();
	forNode->token = forToken;
	forNode->children.push_back(std::move(initNode));
	forNode->children.push_back(std::move(condNode));
	forNode->children.push_back(std::move(updateNode));
	forNode->children.push_back(std::move(bodyNode));

	return forNode;
}
