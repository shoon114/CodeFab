#ifdef _DEBUG
#include "gmock/gmock.h"
#include "AssemblerUnit.h"

using namespace testing;

TEST(AssemblerUnitTest, Parse_CallsWithoutCrashing) {
	AssemblerUnit assembler;
	TokenList tokenList;

	assembler.Parse(tokenList);
}
#endif
