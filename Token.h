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
	KwWhile,
	KwFunc,
	KwReturn,
	KwTrue,
	KwFalse,

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
	Comma,
	Semicolon,

	EndOfFile
};

struct Token {
	TokenType type;
	std::string lexeme;
	int line;
	int column;
};

using TokenList = std::vector<Token>;
