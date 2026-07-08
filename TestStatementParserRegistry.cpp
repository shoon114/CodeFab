#ifdef _DEBUG
#include "gmock/gmock.h"
#include "StatementParserRegistry.h"

using namespace testing;

namespace {

class StubStatementParser : public IStatementParser {
public:
	std::unique_ptr<SyntaxNode> Parse(const TokenList& tokenList, size_t& pos) override {
		return std::make_unique<ProgramNode>();
	}
};

} // namespace

TEST(StatementParserRegistryTest, Resolve_UnregisteredToken_ReturnsNull) {
	std::shared_ptr<IStatementParser> parser = StatementParserRegistry::Instance().Resolve(TokenType::KwElse);

	EXPECT_THAT(parser, IsNull());
}

TEST(StatementParserRegistryTest, Resolve_RegisteredToken_InvokesFactory) {
	StatementParserRegistry::Instance().Register(TokenType::KwReturn, []() {
		return std::make_shared<StubStatementParser>();
	});

	std::shared_ptr<IStatementParser> parser = StatementParserRegistry::Instance().Resolve(TokenType::KwReturn);
	ASSERT_THAT(parser, NotNull());

	TokenList tokenList;
	size_t pos = 0;
	std::unique_ptr<SyntaxNode> node = parser->Parse(tokenList, pos);

	ASSERT_THAT(node, NotNull());
	EXPECT_THAT(node->type, Eq(NodeType::Program));
}
#endif
