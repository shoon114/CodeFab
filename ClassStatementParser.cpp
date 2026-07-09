#include "ClassStatementParser.h"
#include "StatementParserRegistry.h"
#include <stdexcept>
#include <string>

namespace {
	StatementParserRegistrar<ClassStatementParser> registrar(TokenType::KwClass);

	TokenType PeekType(const TokenList& tokenList, size_t pos) {
		return pos < tokenList.size() ? tokenList[pos].type : TokenType::EndOfFile;
	}
}

std::unique_ptr<SyntaxNode> ClassStatementParser::Parse(const TokenList& tokenList, size_t& pos) {
	const Token& classToken = tokenList[pos++]; // 'Class'

	if (PeekType(tokenList, pos) != TokenType::Identifier) {
		throw std::runtime_error("Expected a class name after 'Class' at line " + std::to_string(classToken.line));
	}

	auto classNode = std::make_unique<ClassDeclStmtNode>();
	classNode->token = tokenList[pos++]; // class name

	if (PeekType(tokenList, pos) == TokenType::Colon) {
		pos++; // ':'
		if (PeekType(tokenList, pos) != TokenType::Identifier) {
			throw std::runtime_error("Expected a superclass name after ':' at line " + std::to_string(classNode->token.line));
		}
		// 부모 클래스 이름은 parentNameToken에만 저장한다. children은 메서드
		// 선언(FuncDeclStmtNode)만 담는다고 CheckerUnit/ExecutorUnit이 가정하므로,
		// 여기에 IdentifierNode를 추가하면 그 소비자들이 부모 이름을 메서드로
		// 오인해서 순회하게 된다("본문 없는 메서드" 오류, 잘못된 메서드 등록 등).
		classNode->parentNameToken = tokenList[pos++];
	}

	if (PeekType(tokenList, pos) != TokenType::LBrace) {
		throw std::runtime_error("Expected '{' before class body at line " + std::to_string(classNode->token.line));
	}
	pos++; // '{'

	while (PeekType(tokenList, pos) != TokenType::RBrace) {
		if (PeekType(tokenList, pos) == TokenType::EndOfFile) {
			throw std::runtime_error("Expected '}' to close class body at line " + std::to_string(classNode->token.line));
		}
		classNode->children.push_back(ParseMethod(tokenList, pos));
	}

	pos++; // '}'
	return classNode;
}

std::unique_ptr<SyntaxNode> ClassStatementParser::ParseMethod(const TokenList& tokenList, size_t& pos) {
	if (PeekType(tokenList, pos) != TokenType::Identifier) {
		throw std::runtime_error("Expected a method name inside class body at line " + std::to_string(tokenList[pos].line));
	}

	const Token& nameToken = tokenList[pos++];
	auto methodNode = std::make_unique<FuncDeclStmtNode>();
	methodNode->token = nameToken;

	if (PeekType(tokenList, pos) != TokenType::LParen) {
		throw std::runtime_error("Expected '(' after method name '" + nameToken.lexeme + "' at line " + std::to_string(nameToken.line));
	}
	pos++; // '('

	if (PeekType(tokenList, pos) != TokenType::RParen) {
		while (true) {
			if (PeekType(tokenList, pos) != TokenType::Identifier) {
				throw std::runtime_error("Expected a parameter name at line " + std::to_string(nameToken.line));
			}
			auto paramNode = std::make_unique<IdentifierNode>();
			paramNode->token = tokenList[pos++];
			methodNode->children.push_back(std::move(paramNode));

			if (PeekType(tokenList, pos) == TokenType::Comma) {
				pos++; // ','
			} else {
				break;
			}
		}
	}

	if (PeekType(tokenList, pos) != TokenType::RParen) {
		throw std::runtime_error("Expected ')' after method parameters at line " + std::to_string(nameToken.line));
	}
	pos++; // ')'

	std::shared_ptr<IStatementParser> bodyParser = StatementParserRegistry::Instance().Resolve(PeekType(tokenList, pos));
	if (bodyParser == nullptr) {
		throw std::runtime_error("Expected '{' before method body at line " + std::to_string(nameToken.line));
	}
	methodNode->children.push_back(bodyParser->Parse(tokenList, pos));

	return methodNode;
}
