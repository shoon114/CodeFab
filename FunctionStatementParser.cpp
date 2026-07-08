#include "FunctionStatementParser.h"
#include "StatementParserRegistry.h"

namespace {
	StatementParserRegistrar<FunctionStatementParser> registrar(TokenType::KwFunc);
}

std::unique_ptr<SyntaxNode> FunctionStatementParser::Parse(const TokenList& tokenList, size_t& pos) {
	pos++; // 'func'

	const Token& nameToken = tokenList[pos++]; // 함수 이름

	auto funcNode = std::make_unique<FuncDeclStmtNode>();
	funcNode->token = nameToken;

	pos++; // '('

	while (tokenList[pos].type != TokenType::RParen) {
		if (tokenList[pos].type == TokenType::Comma) {
			pos++;
			continue;
		}

		auto paramNode = std::make_unique<IdentifierNode>();
		paramNode->token = tokenList[pos++];
		funcNode->children.push_back(std::move(paramNode));
	}
	pos++; // ')'

	// 함수 본문 '{...}'는 이미 등록되어 있는 파서(BlockParser)에게 위임한다.
	// IfStatementParser가 조건식 파서를 레지스트리에서 얻어오는 것과 같은 방식.
	std::shared_ptr<IStatementParser> bodyParser = StatementParserRegistry::Instance().Resolve(tokenList[pos].type);
	funcNode->children.push_back(bodyParser->Parse(tokenList, pos));

	return funcNode;
}
