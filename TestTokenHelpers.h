#pragma once
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
