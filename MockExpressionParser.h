#pragma once
#include "gmock/gmock.h"
#include "IExpressionParser.h"

// The real ExpressionParser implementation belongs to another module/owner,
// so this mock stands in wherever IExpressionParser is depended on for testing.
class MockExpressionParser : public IExpressionParser {
public:
	MOCK_METHOD(std::unique_ptr<SyntaxNode>, Parse, (const TokenList& tokenList, size_t& pos), (override));
};
