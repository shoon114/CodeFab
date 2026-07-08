#include "FunctionStatementParser.h"
#include "StatementParserRegistry.h"
#include <stdexcept>
#include <string>

namespace {
	StatementParserRegistrar<FunctionStatementParser> registrar(TokenType::KwFunc);
}

std::unique_ptr<SyntaxNode> FunctionStatementParser::Parse(const TokenList& tokenList, size_t& pos) {
	const Token& funcToken = tokenList[pos++];

	if (pos >= tokenList.size() || tokenList[pos].type != TokenType::Identifier) {
		throw std::runtime_error("Expected a function name after 'func' at line " + std::to_string(funcToken.line));
	}
	const Token& nameToken = tokenList[pos++]; 

	auto funcNode = std::make_unique<FuncDeclStmtNode>();
	funcNode->token = nameToken;

	if (pos >= tokenList.size() || tokenList[pos].type != TokenType::LParen) {
		throw std::runtime_error("Invalid Syntax. '(' is Missing");
	}
	pos++; 

	if (pos < tokenList.size() && tokenList[pos].type != TokenType::RParen) {
		while (true) {
			if (pos >= tokenList.size() || tokenList[pos].type != TokenType::Identifier) {
				throw std::runtime_error("Expected a parameter name at line " + std::to_string(nameToken.line));
			}

			auto paramNode = std::make_unique<IdentifierNode>();
			paramNode->token = tokenList[pos++];
			funcNode->children.push_back(std::move(paramNode));

			if (pos < tokenList.size() && tokenList[pos].type == TokenType::Comma) {
				pos++;
				continue;
			}
			break;
		}
	}

	if (pos >= tokenList.size() || tokenList[pos].type != TokenType::RParen) {
		throw std::runtime_error("Invalid Syntax. ')' is Missing");
	}
	pos++;

	std::shared_ptr<IStatementParser> bodyParser =
		pos < tokenList.size() ? StatementParserRegistry::Instance().Resolve(tokenList[pos].type) : nullptr;
	if (bodyParser == nullptr) {
		throw std::runtime_error("Expected a function body at line " + std::to_string(nameToken.line));
	}
	funcNode->children.push_back(bodyParser->Parse(tokenList, pos));

	return funcNode;
}
