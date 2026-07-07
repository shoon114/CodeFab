#pragma once
#include "gmock/gmock.h"
#include "IStatementParser.h"

// Stands in for whatever IStatementParser gets resolved via
// StatementParserRegistry for a given leading token (e.g. an ExprStmtParser
// registered for Identifier), so callers that delegate through the registry
// can be tested without depending on that parser's real implementation.
class MockStatementParser : public IStatementParser {
public:
	MOCK_METHOD(std::unique_ptr<SyntaxNode>, Parse, (const TokenList& tokenList, size_t& pos), (override));
};
