#include "gmock/gmock.h"
#include "AssemblyUnitIf.cpp"

using namespace testing;

class MockAssemblerUnit : public AssemblerUnit {
public:
    MOCK_METHOD(TokenList, Tokenize, (const std::string& source), (override));
    MOCK_METHOD(std::unique_ptr<SyntaxTree>, Parse, (const TokenList& tokens), (override));
};

TEST(AssemblyUnitIfTest, Tokenize_MockedBySourceCode_ReturnsGivenTokens) {
    MockAssemblerUnit mockAssembler;
    TokenList expectedTokens = {
        Token{ TokenType::Keyword,    "if", 1, 1 },
        Token{ TokenType::Operator,   "(",  1, 4 },
        Token{ TokenType::Identifier, "a",  1, 5 },
        Token{ TokenType::Operator,   "<",  1, 7 },
        Token{ TokenType::Number,     "10", 1, 9 },
        Token{ TokenType::Operator,   ")",  1, 11 },
        Token{ TokenType::EndOfFile,  "",   1, 12 },
    };

    EXPECT_CALL(mockAssembler, Tokenize("if (a < 10)"))
        .WillOnce(Return(expectedTokens));

    TokenList tokens = mockAssembler.Tokenize("if (a < 10)");

    ASSERT_EQ(tokens.size(), expectedTokens.size());
    EXPECT_EQ(tokens[0].type, TokenType::Keyword);
    EXPECT_EQ(tokens[0].lexeme, "if");
    EXPECT_EQ(tokens.back().type, TokenType::EndOfFile);
}

