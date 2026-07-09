#include <stdexcept>
#include <string>
#include "IfStatementParser.h"
#include "StatementParserRegistry.h"

namespace {
	StatementParserRegistrar<IfStatementParser> registrar(TokenType::KwIf);

	TokenType PeekType(const TokenList& tokenList, size_t pos) {
		return pos < tokenList.size() ? tokenList[pos].type : TokenType::EndOfFile;
	}

	int PeekLine(const TokenList& tokenList, size_t pos) {
		return pos < tokenList.size() ? tokenList[pos].line : tokenList.back().line;
	}

	std::unique_ptr<SyntaxNode> ResolveAndParse(const TokenList& tokenList, size_t& pos, const char* what) {
		std::shared_ptr<IStatementParser> parser = StatementParserRegistry::Instance().Resolve(PeekType(tokenList, pos));
		if (parser == nullptr) {
			throw std::runtime_error(std::string("Expected ") + what + " at line " + std::to_string(PeekLine(tokenList, pos)));
		}
		return parser->Parse(tokenList, pos);
	}

	void ExpectToken(const TokenList& tokenList, size_t& pos, TokenType expected, const char* what) {
		if (PeekType(tokenList, pos) != expected) {
			throw std::runtime_error(std::string("Expected ") + what + " at line " + std::to_string(PeekLine(tokenList, pos)));
		}
		pos++;
	}

	std::unique_ptr<SyntaxNode> ParseBody(const TokenList& tokenList, size_t& pos, const char* context) {
		auto bodyNode = ResolveAndParse(tokenList, pos, (std::string("a statement or '{' to start ") + context).c_str());

		if (PeekType(tokenList, pos) == TokenType::Semicolon) {
			pos++;
		}

		if (bodyNode->type != NodeType::BlockStmt) {
			auto blockNode = std::make_unique<BlockStmtNode>();
			blockNode->token = bodyNode->token;
			blockNode->children.push_back(std::move(bodyNode));
			return blockNode;
		}

		return bodyNode;
	}
}

std::unique_ptr<SyntaxNode> IfStatementParser::Parse(const TokenList& tokenList, size_t& pos) {
	const Token& ifToken = tokenList[pos++];

	ExpectToken(tokenList, pos, TokenType::LParen, "'(' after 'if'");

	auto conditionNode = ResolveAndParse(tokenList, pos, "a condition expression in 'if'");

	ExpectToken(tokenList, pos, TokenType::RParen, "')' after if-condition");

	auto thenNode = ParseBody(tokenList, pos, "if-body");

	auto ifNode = std::make_unique<IfStmtNode>();
	ifNode->token = ifToken;
	ifNode->children.push_back(std::move(conditionNode));
	ifNode->children.push_back(std::move(thenNode));

	if (PeekType(tokenList, pos) == TokenType::KwElse) {
		pos++;

		if (PeekType(tokenList, pos) == TokenType::KwIf) {
			ifNode->children.push_back(ResolveAndParse(tokenList, pos, "an 'if' after 'else'"));
		} else {
			ifNode->children.push_back(ParseBody(tokenList, pos, "else-body"));
		}
	}

	return ifNode;
}
