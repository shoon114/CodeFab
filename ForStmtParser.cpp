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

	// '{}' body는 BlockParser가 이미 BlockStmtNode로 감싸서 ExecutorUnit의
	// ScopeGuard(RAII)로 스코프가 격리된다. '{}' 없는 단일 문장 body는 그
	// 격리를 받지 못하므로, 여기서 직접 BlockStmtNode 하나로 감싸 동일하게
	// 스코프가 생기도록 한다 -- 이렇게 하면 ExecutorUnit/CheckerUnit은 전혀
	// 손댈 필요 없이 '{}' body와 똑같은 경로로 처리된다.
	if (bodyNode->type != NodeType::BlockStmt) {
		auto blockNode = std::make_unique<BlockStmtNode>();
		blockNode->token = bodyNode->token;
		blockNode->children.push_back(std::move(bodyNode));
		bodyNode = std::move(blockNode);
	}

	auto forNode = std::make_unique<ForStmtNode>();
	forNode->token = forToken;
	forNode->children.push_back(std::move(initNode));
	forNode->children.push_back(std::move(condNode));
	forNode->children.push_back(std::move(updateNode));
	forNode->children.push_back(std::move(bodyNode));

	return forNode;
}
