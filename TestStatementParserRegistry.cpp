#ifdef _DEBUG
#include "gmock/gmock.h"
#include "StatementParserRegistry.h"

using namespace testing;

namespace {

class StubStatementParser : public IStatementParser {
public:
	std::unique_ptr<SyntaxNode> Parse(const TokenList& tokenList, size_t& pos) override {
		auto node = std::make_unique<SyntaxNode>();
		node->type = NodeType::Program;
		return node;
	}
};

class StubExpressionParser : public IExpressionParser {
public:
	std::unique_ptr<SyntaxNode> Parse(const TokenList& tokenList, size_t& pos) override {
		return nullptr;
	}
};

} // namespace

TEST(StatementParserRegistryTest, Resolve_UnregisteredToken_ReturnsNull) {
	StubExpressionParser exprParser;

	std::shared_ptr<IStatementParser> parser = StatementParserRegistry::Instance().Resolve(TokenType::KwWhile, exprParser);

	EXPECT_THAT(parser, IsNull());
}

TEST(StatementParserRegistryTest, Resolve_RegisteredToken_InvokesFactoryWithExprParser) {
	StubExpressionParser exprParser;
	StatementParserRegistry::Instance().Register(TokenType::KwReturn, [](IExpressionParser&) {
		return std::make_shared<StubStatementParser>();
	});

	std::shared_ptr<IStatementParser> parser = StatementParserRegistry::Instance().Resolve(TokenType::KwReturn, exprParser);
	ASSERT_THAT(parser, NotNull());

	TokenList tokenList;
	size_t pos = 0;
	std::unique_ptr<SyntaxNode> node = parser->Parse(tokenList, pos);

	ASSERT_THAT(node, NotNull());
	EXPECT_THAT(node->type, Eq(NodeType::Program));
}
#endif
