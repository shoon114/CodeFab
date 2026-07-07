#ifdef _DEBUG
#include "gmock/gmock.h"
#include "BlockParser.h"
#include "MockExpressionParser.h"
#include "TestTokenHelpers.h"

using namespace testing;

class BlockParserTest : public Test {
public:
	TokenList MakeBlockEmptyBlock() {
		return TokenList{
			MakeToken(TokenType::LBrace, "{", 0, 0),
			MakeToken(TokenType::RBrace, "}", 0, 2)
		};
	}
protected:
	MockExpressionParser exprParser;
	BlockParser parser{ exprParser };
};

TEST_F(BlockParserTest, Parse_EmptyBlock) {
	// "{ }"
	TokenList tokenList = MakeBlockEmptyBlock();
	size_t pos = 0;

	std::unique_ptr<SyntaxNode> node = parser.Parse(tokenList, pos);
}
#endif
