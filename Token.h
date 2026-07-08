#pragma once
#include <string>
#include <vector>

enum class TokenType {
	Number,
	String,
	Identifier,

	KwVar,
	KwIf,
	KwElse,
	KwFor,
	KwFunc,
	KwReturn,
	KwTrue,
	KwFalse,
	Print,

	Plus,
	Minus,
	Star,
	Slash,
	Percent,
	Assign,
	Eq,
	NotEq,
	Lt,
	Gt,
	LtEq,
	GtEq,
	And,
	Or,
	Not,

	LParen,
	RParen,
	LBrace,
	RBrace,
	LBracket,
	RBracket,
	Comma,
	Semicolon,

	EndOfFile
};

struct Token{
	TokenType type;
	std::string lexeme;
    double realValue;
    std::string originalCode;
	int line;
	int column;
};

using TokenList = std::vector<Token>;
