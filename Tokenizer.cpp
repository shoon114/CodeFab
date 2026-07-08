#include "Tokenizer.h"
#include <iostream>
#include <sstream>
#include <cctype>
#include <unordered_map>

namespace {
	const std::unordered_map<std::string, TokenType> keywords = {
		{"var", TokenType::KwVar}, {"if", TokenType::KwIf}, {"else", TokenType::KwElse},
		{"for", TokenType::KwFor}, {"func", TokenType::KwFunc}, {"return", TokenType::KwReturn},
		{"true", TokenType::KwTrue}, {"false", TokenType::KwFalse}, {"print", TokenType::Print},
	};
	const std::unordered_map<std::string, TokenType> operators = {
		{"+", TokenType::Plus}, {"-", TokenType::Minus}, {"*", TokenType::Star}, {"/", TokenType::Slash},
		{"%", TokenType::Percent}, {"=", TokenType::Assign}, {"==", TokenType::Eq}, {"!=", TokenType::NotEq},
		{"<", TokenType::Lt}, {">", TokenType::Gt}, {"<=", TokenType::LtEq}, {">=", TokenType::GtEq},
		{"(", TokenType::LParen}, {")", TokenType::RParen}, {"{", TokenType::LBrace}, {"}", TokenType::RBrace},
		{",", TokenType::Comma}, {";", TokenType::Semicolon},
	};
}

void Tokenizer::GetCodeFromUser()
{
	std::ostringstream buffer;
	buffer << std::cin.rdbuf();
	originalCode = buffer.str();
}

bool Tokenizer::CanExtendToTwoCharOperator(char c)
{
	return c == '=' || c == '!' || c == '<' || c == '>';
}

bool Tokenizer::IsSingleCharPunctuation(char c)
{
	static const std::string punctuation = "(){};,+-*/%";
	return punctuation.find(c) != std::string::npos;
}

std::vector<Token> Tokenizer::ScanWords()
{
	std::vector<Token> words;
	size_t i = 0;
	const size_t n = originalCode.size();
	int line = 0;
	int column = 0;

	auto addWord = [&](const std::string& text) {
		Token word;
		word.type = TokenType::Identifier; // 아직 분류 전, ClassifyToken에서 확정됨
		word.lexeme = text;
		word.realValue = 0.0;
		word.originalCode = originalCode;
		word.line = line;
		word.column = column;
		words.push_back(word);
		column++;
	};

	while (i < n) {
		char c = originalCode[i];

		if (c == '\n') {
			line++;
			column = 0;
			i++;
			continue;
		}

		if (std::isspace(static_cast<unsigned char>(c))) {
			i++;
			continue;
		}

		if (c == '"') {
			size_t start = i++;
			while (i < n && originalCode[i] != '"') {
				i++;
			}
			if (i < n) {
				i++; // closing quote
			}
			addWord(originalCode.substr(start, i - start));
			continue;
		}

		if (CanExtendToTwoCharOperator(c)) {
			bool isTwoCharOperator = (i + 1 < n && originalCode[i + 1] == '=');
			addWord(originalCode.substr(i, isTwoCharOperator ? 2 : 1));
			i += isTwoCharOperator ? 2 : 1;
			continue;
		}

		if (IsSingleCharPunctuation(c)) {
			addWord(std::string(1, c));
			i++;
			continue;
		}

		size_t start = i;
		while (i < n
			&& !std::isspace(static_cast<unsigned char>(originalCode[i]))
			&& originalCode[i] != '"'
			&& !CanExtendToTwoCharOperator(originalCode[i])
			&& !IsSingleCharPunctuation(originalCode[i])) {
			i++;
		}
		addWord(originalCode.substr(start, i - start));
	}

	return words;
}

std::vector<std::string> Tokenizer::SplitIntoWords()
{
	std::vector<std::string> words;
	for (const Token& word : ScanWords()) {
		words.push_back(word.lexeme);
	}
	return words;
}

void Tokenizer::ClassifyToken(Token& token)
{
	const std::string& raw = token.lexeme;

	if (raw.size() >= 2 && raw.front() == '"' && raw.back() == '"') {
		token.type = TokenType::String;
		token.lexeme = raw.substr(1, raw.size() - 2);
		return;
	}

	if (std::isdigit(static_cast<unsigned char>(raw[0]))) {
		token.type = TokenType::Number;
		token.realValue = std::stod(raw);
		return;
	}

	auto keywordIt = keywords.find(raw);
	if (keywordIt != keywords.end()) {
		token.type = keywordIt->second;
		return;
	}

	auto operatorIt = operators.find(raw);
	if (operatorIt != operators.end()) {
		token.type = operatorIt->second;
		return;
	}

	token.type = TokenType::Identifier;
}

TokenList Tokenizer::CreateTokenForCode()
{
	TokenList tokens = ScanWords();

	for (Token& token : tokens) {
		ClassifyToken(token);
	}

	Token eof;
	eof.type = TokenType::EndOfFile;
	eof.lexeme = "";
	eof.realValue = 0.0;
	eof.originalCode = originalCode;
	eof.line = tokens.empty() ? 0 : tokens.back().line;
	eof.column = tokens.empty() ? 0 : tokens.back().column + 1;
	tokens.push_back(eof);

	return tokens;
}
