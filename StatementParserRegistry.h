#pragma once
#include <functional>
#include <memory>
#include <unordered_map>
#include "Token.h"
#include "IStatementParser.h"
#include "IExpressionParser.h"

using StatementParserFactory = std::function<std::shared_ptr<IStatementParser>(IExpressionParser&)>;

// Global singleton. Each IStatementParser implementation self-registers by
// declaring a file-scope StatementParserRegistrar<T> in its own .cpp.
// AssemblerUnit never needs to know which parsers exist; it only queries this registry.
class StatementParserRegistry {
public:
	static StatementParserRegistry& Instance();

	void Register(TokenType leadingToken, StatementParserFactory factory);
	std::shared_ptr<IStatementParser> Resolve(TokenType leadingToken, IExpressionParser& exprParser) const;

private:
	std::unordered_map<TokenType, StatementParserFactory> factories;
};

// To add a new statement parser, add a single line like below in your own
// .cpp file. No need to touch AssemblerUnit or any other parser's code.
//
//   namespace {
//   StatementParserRegistrar<MyStatementParser> registrar(TokenType::KwMyKeyword);
//   }
template <typename TParser>
class StatementParserRegistrar {
public:
	explicit StatementParserRegistrar(TokenType leadingToken) {
		StatementParserRegistry::Instance().Register(leadingToken, [](IExpressionParser& exprParser) {
			return std::make_shared<TParser>(exprParser);
		});
	}
};
