#pragma once
#include <utility>
#include <vector>
#include "Token.h"

inline Token MakeToken(TokenType type, const std::string& lexeme, int line, int column) {
	Token token;
	token.type = type;
	token.lexeme = lexeme;
	token.realValue = 0.0;
	token.originalCode = lexeme;
	token.line = line;
	token.column = column;
	return token;
}

// 토큰이 등장하는 순서를 그대로 column으로 매기고, line은 항상 1로 둔다.
inline TokenList MakeTokens(std::initializer_list<std::pair<TokenType, std::string>> tokens) {
	TokenList result;
	int column = 0;
	for (const auto& [type, lexeme] : tokens) {
		result.push_back(MakeToken(type, lexeme, 1, column++));
	}
	return result;
}
