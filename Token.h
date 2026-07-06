#pragma once
#include <string>
#include <vector>

enum class TokenType { Identifier, Number, Operator, Keyword, EndOfFile /* ... */ };

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;
};

using TokenList = std::vector<Token>;
