#include "Tokenizer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>
#include <unordered_map>
#include <algorithm>
#include <stdexcept>

namespace { 
	const std::unordered_map<std::string, TokenType> keywords = {
		{"var", TokenType::KwVar}, {"if", TokenType::KwIf}, {"else", TokenType::KwElse},
		{"for", TokenType::KwFor}, {"func", TokenType::KwFunc}, {"return", TokenType::KwReturn},
		{"true", TokenType::KwTrue}, {"false", TokenType::KwFalse}, {"print", TokenType::Print},
		{"class", TokenType::KwClass}, {"this", TokenType::KwThis}, {"super", TokenType::KwSuper},
		{"instanceof", TokenType::KwInstanceof},
		{"import", TokenType::KwImport}, {"alias", TokenType::KwAlias},
	};
	const std::unordered_map<std::string, TokenType> operators = {
		{"+", TokenType::Plus}, {"-", TokenType::Minus}, {"*", TokenType::Star}, {"/", TokenType::Slash},
		{"%", TokenType::Percent}, {"=", TokenType::Assign}, {"==", TokenType::Eq}, {"!=", TokenType::NotEq},
		{"<", TokenType::Lt}, {">", TokenType::Gt}, {"<=", TokenType::LtEq}, {">=", TokenType::GtEq},
		{"(", TokenType::LParen}, {")", TokenType::RParen}, {"{", TokenType::LBrace}, {"}", TokenType::RBrace},
		{"[", TokenType::LBracket}, {"]", TokenType::RBracket},
		{",", TokenType::Comma}, {";", TokenType::Semicolon}, {".", TokenType::Dot}, {":", TokenType::Colon},
		{"!", TokenType::Not}, {"&&", TokenType::And}, {"||", TokenType::Or},
	};

	std::string ToLower(const std::string& text) {
		std::string lowered = text;
		std::transform(lowered.begin(), lowered.end(), lowered.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return lowered;
	}
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

bool Tokenizer::IsDoubledOperatorStart(char c)
{
	return c == '&' || c == '|';
}

bool Tokenizer::IsSingleCharPunctuation(char c)
{
	static const std::string punctuation = "(){}[];,+-*/%.:";
	return punctuation.find(c) != std::string::npos;
}

std::vector<Token> Tokenizer::ScanWords(const std::string& source)
{
	std::vector<Token> words;
	size_t i = 0;
	const size_t n = source.size();
	int line = 0;
	int column = 0;

	auto addWord = [&](const std::string& text) {
		Token word;
		word.type = TokenType::Identifier; // 아직 분류 전, ClassifyToken에서 확정됨
		word.lexeme = text;
		word.realValue = 0.0;
		word.originalCode = source;
		word.line = line;
		word.column = column;
		words.push_back(word);
		column++;
	};

	while (i < n) {
		char c = source[i];

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
			while (i < n && source[i] != '"') {
				i++;
			}
			if (i < n) {
				i++; // closing quote
			}
			addWord(source.substr(start, i - start));
			continue;
		}

		if (std::isdigit(static_cast<unsigned char>(c))) {
			size_t start = i;
			while (i < n && std::isdigit(static_cast<unsigned char>(source[i]))) {
				i++;
			}
			// '.'을 맴버 접근 연산자(Dot)로도 쓰기 때문에, 소수점은 뒤에 숫자가 붙어있을 때만 흡수한다.
			if (i + 1 < n && source[i] == '.' && std::isdigit(static_cast<unsigned char>(source[i + 1]))) {
				i++;
				while (i < n && std::isdigit(static_cast<unsigned char>(source[i]))) {
					i++;
				}
			}
			addWord(source.substr(start, i - start));
			continue;
		}

		if (CanExtendToTwoCharOperator(c)) {
			bool isTwoCharOperator = (i + 1 < n && source[i + 1] == '=');
			addWord(source.substr(i, isTwoCharOperator ? 2 : 1));
			i += isTwoCharOperator ? 2 : 1;
			continue;
		}

		if (IsDoubledOperatorStart(c)) {
			// '&&', '||'만 유효한 연산자다. 짝이 안 맞는 단독 '&'/'|'는 언어에서
			// 정의되지 않은 문자이지만, 토크나이저는 그냥 한 글자로 떼어서 넘긴다
			// (분류는 ClassifyToken이 Identifier로 처리).
			bool isDoubled = (i + 1 < n && source[i + 1] == c);
			addWord(source.substr(i, isDoubled ? 2 : 1));
			i += isDoubled ? 2 : 1;
			continue;
		}

		if (IsSingleCharPunctuation(c)) {
			addWord(std::string(1, c));
			i++;
			continue;
		}

		size_t start = i;
		while (i < n
			&& !std::isspace(static_cast<unsigned char>(source[i]))
			&& source[i] != '"'
			&& !CanExtendToTwoCharOperator(source[i])
			&& !IsDoubledOperatorStart(source[i])
			&& !IsSingleCharPunctuation(source[i])) {
			i++;
		}
		addWord(source.substr(start, i - start));
	}

	return words;
}

std::vector<std::string> Tokenizer::SplitIntoWords()
{
	std::vector<std::string> words;
	for (const Token& word : ScanWords(originalCode)) {
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

	auto keywordIt = keywords.find(ToLower(raw));
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

std::string Tokenizer::ReadFile(const std::string& path, int line)
{
	std::ifstream file(path);
	if (!file) {
		throw std::runtime_error("Cannot open import file: " + path + " at line " + std::to_string(line));
	}
	std::ostringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

TokenList Tokenizer::TokenizeFileForImport(const std::string& path, int line, std::vector<std::string>& activeImports)
{
	// ModuleLoader가 나중에 도입되면 순환 import 감지 책임은 그쪽으로 옮겨간다.
	// 지금은 Tokenizer가 직접 재귀적으로 파일을 이어붙이기 때문에, 스택 오버플로우를
	// 막기 위한 최소한의 가드만 여기 둔다.
	if (std::find(activeImports.begin(), activeImports.end(), path) != activeImports.end()) {
		throw std::runtime_error("Circular import detected: " + path + " at line " + std::to_string(line));
	}

	std::string fileSource = ReadFile(path, line);

	activeImports.push_back(path);

	TokenList importedTokens = ScanWords(fileSource);
	for (Token& token : importedTokens) {
		ClassifyToken(token);
	}
	ResolveImports(importedTokens, activeImports);

	activeImports.pop_back();

	return importedTokens;
}

void Tokenizer::ResolveImports(TokenList& tokens, std::vector<std::string>& activeImports)
{
	for (size_t i = 0; i < tokens.size(); i++) {
		if (tokens[i].type != TokenType::KwImport || i + 1 >= tokens.size() || tokens[i + 1].type != TokenType::String) {
			continue;
		}

		size_t semicolonIndex = i + 1;
		while (semicolonIndex < tokens.size() && tokens[semicolonIndex].type != TokenType::Semicolon) {
			semicolonIndex++;
		}
		size_t insertPos = (semicolonIndex < tokens.size()) ? semicolonIndex + 1 : tokens.size();

		TokenList importedTokens = TokenizeFileForImport(tokens[i + 1].lexeme, tokens[i].line, activeImports);

		tokens.insert(tokens.begin() + insertPos, importedTokens.begin(), importedTokens.end());
		i = insertPos + importedTokens.size() - 1; // 삽입된 토큰은 이미 재귀적으로 처리됐으니 건너뛴다
	}
}

TokenList Tokenizer::CreateTokenForCode()
{
	TokenList tokens = ScanWords(originalCode);

	for (Token& token : tokens) {
		ClassifyToken(token);
	}

	std::vector<std::string> activeImports;
	ResolveImports(tokens, activeImports);

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
