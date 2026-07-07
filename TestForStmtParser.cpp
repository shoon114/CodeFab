#ifdef _DEBUG
#include "gmock/gmock.h"
#include "ForStmtParser.h"
#include "TestTokenHelpers.h"

using namespace testing;

class ForStmtParserTest : public Test {
protected:
	ForStmtParser parser;
};

TEST_F(ForStmtParserTest, Parse_CallsWithoutCrashing) {
	TokenList tokenList;
	size_t pos = 0;

	parser.Parse(tokenList, pos);
}
#endif
