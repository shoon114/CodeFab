#include "Tokenizer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cctype>
#include <unordered_map>
#include <algorithm>
#include <stdexcept>
#include <filesystem>

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

	bool IsTwoCharOperatorStart(char c) {
		return c == '=' || c == '!' || c == '<' || c == '>';
	}

	bool IsDoubledOperatorStart(char c) {
		return c == '&' || c == '|';
	}

	bool IsSingleCharPunctuation(char c) {
		static const std::string punctuation = "(){}[];,+-*/%.:";
		return punctuation.find(c) != std::string::npos;
	}

	// ScanWords()가 현재 위치의 문자 하나를 보고 "이 종류(문자열/숫자/연산자/구두점)를
	// 내가 처리할 수 있다"고 판단하는 전략들의 공통 인터페이스. 각 문자 종류는 서로
	// 배타적이라(숫자 시작 문자와 연산자 시작 문자가 겹치지 않음) 등록 순서와 무관하게
	// 정확히 하나의 전략만 CanHandle을 true로 반환한다.
	class ITokenScanner {
	public:
		virtual ~ITokenScanner() = default;
		virtual bool CanHandle(const std::string& source, size_t pos) const = 0;
		// pos를 소비한 만큼 전진시키고, 소비한 구간의 원문 텍스트를 반환한다.
		virtual std::string Scan(const std::string& source, size_t& pos) const = 0;
	};

	// TokenType -> IStatementParser 매핑을 쓰는 StatementParserRegistry와 동일한
	// 자기 등록 방식. 다만 여기서는 키가 TokenType 하나가 아니라 "이 문자를 처리할
	// 수 있는가"라는 술어라서, Resolve()는 등록된 전략들을 순서대로 물어보고 첫
	// 번째로 CanHandle==true인 것을 돌려준다.
	class TokenScannerRegistry {
	public:
		static TokenScannerRegistry& Instance() {
			static TokenScannerRegistry instance;
			return instance;
		}

		void Register(std::unique_ptr<ITokenScanner> scanner) {
			scanners.push_back(std::move(scanner));
		}

		const ITokenScanner* Resolve(const std::string& source, size_t pos) const {
			for (const std::unique_ptr<ITokenScanner>& scanner : scanners) {
				if (scanner->CanHandle(source, pos)) {
					return scanner.get();
				}
			}
			return nullptr;
		}

	private:
		std::vector<std::unique_ptr<ITokenScanner>> scanners;
	};

	template <typename TScanner>
	struct TokenScannerRegistrar {
		TokenScannerRegistrar() {
			TokenScannerRegistry::Instance().Register(std::make_unique<TScanner>());
		}
	};

	class StringLiteralScanner : public ITokenScanner {
	public:
		bool CanHandle(const std::string& source, size_t pos) const override {
			return source[pos] == '"';
		}
		std::string Scan(const std::string& source, size_t& pos) const override {
			size_t start = pos++;
			while (pos < source.size() && source[pos] != '"') {
				pos++;
			}
			if (pos < source.size()) {
				pos++; // closing quote
			}
			return source.substr(start, pos - start);
		}
	};
	TokenScannerRegistrar<StringLiteralScanner> stringLiteralScannerRegistrar;

	class NumberScanner : public ITokenScanner {
	public:
		bool CanHandle(const std::string& source, size_t pos) const override {
			return std::isdigit(static_cast<unsigned char>(source[pos])) != 0;
		}
		std::string Scan(const std::string& source, size_t& pos) const override {
			const size_t n = source.size();
			size_t start = pos;
			while (pos < n && std::isdigit(static_cast<unsigned char>(source[pos]))) {
				pos++;
			}
			// '.'을 맴버 접근 연산자(Dot)로도 쓰기 때문에, 소수점은 뒤에 숫자가 붙어있을 때만 흡수한다.
			if (pos + 1 < n && source[pos] == '.' && std::isdigit(static_cast<unsigned char>(source[pos + 1]))) {
				pos++;
				while (pos < n && std::isdigit(static_cast<unsigned char>(source[pos]))) {
					pos++;
				}
			}
			return source.substr(start, pos - start);
		}
	};
	TokenScannerRegistrar<NumberScanner> numberScannerRegistrar;

	class TwoCharOperatorScanner : public ITokenScanner {
	public:
		bool CanHandle(const std::string& source, size_t pos) const override {
			return IsTwoCharOperatorStart(source[pos]);
		}
		std::string Scan(const std::string& source, size_t& pos) const override {
			bool isTwoCharOperator = (pos + 1 < source.size() && source[pos + 1] == '=');
			std::string text = source.substr(pos, isTwoCharOperator ? 2 : 1);
			pos += isTwoCharOperator ? 2 : 1;
			return text;
		}
	};
	TokenScannerRegistrar<TwoCharOperatorScanner> twoCharOperatorScannerRegistrar;

	class DoubledOperatorScanner : public ITokenScanner {
	public:
		bool CanHandle(const std::string& source, size_t pos) const override {
			return IsDoubledOperatorStart(source[pos]);
		}
		std::string Scan(const std::string& source, size_t& pos) const override {
			// '&&', '||'만 유효한 연산자다. 짝이 안 맞는 단독 '&'/'|'는 언어에서
			// 정의되지 않은 문자이지만, 토크나이저는 그냥 한 글자로 떼어서 넘긴다
			// (분류는 ClassifyToken이 Identifier로 처리).
			bool isDoubled = (pos + 1 < source.size() && source[pos + 1] == source[pos]);
			std::string text = source.substr(pos, isDoubled ? 2 : 1);
			pos += isDoubled ? 2 : 1;
			return text;
		}
	};
	TokenScannerRegistrar<DoubledOperatorScanner> doubledOperatorScannerRegistrar;

	class PunctuationScanner : public ITokenScanner {
	public:
		bool CanHandle(const std::string& source, size_t pos) const override {
			return IsSingleCharPunctuation(source[pos]);
		}
		std::string Scan(const std::string& source, size_t& pos) const override {
			return std::string(1, source[pos++]);
		}
	};
	TokenScannerRegistrar<PunctuationScanner> punctuationScannerRegistrar;

	// 위 전략들 중 아무도 처리하지 못하는 나머지(식별자/키워드)는 레지스트리에
	// 등록하지 않고, Resolve()가 nullptr을 돌려줄 때 ScanWords()가 직접 처리하는
	// 기본 동작으로 둔다 -- "항상 처리 가능"한 전략을 굳이 만들어서 다른 전략들보다
	// 먼저 등록되지 않게 순서를 신경 쓰는 것보다 이게 더 단순하고 안전하다.
	std::string ScanIdentifier(const std::string& source, size_t& pos) {
		const size_t n = source.size();
		size_t start = pos;
		while (pos < n
			&& !std::isspace(static_cast<unsigned char>(source[pos]))
			&& source[pos] != '"'
			&& !IsTwoCharOperatorStart(source[pos])
			&& !IsDoubledOperatorStart(source[pos])
			&& !IsSingleCharPunctuation(source[pos])) {
			pos++;
		}
		return source.substr(start, pos - start);
	}
}

void Tokenizer::GetCodeFromUser()
{
	std::ostringstream buffer;
	buffer << std::cin.rdbuf();
	originalCode = buffer.str();
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

		const ITokenScanner* scanner = TokenScannerRegistry::Instance().Resolve(source, i);
		addWord(scanner != nullptr ? scanner->Scan(source, i) : ScanIdentifier(source, i));
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
	// "abc.txt"와 "./abc.txt"처럼 같은 파일을 다르게 표기해도 같은 파일로 인식되도록,
	// 순환 감지 비교는 정규화된 경로로 한다. 파일을 실제로 여는 동작은 그대로 원본
	// path를 사용한다 (정규화/캐싱 등 파일 접근 자체의 정책은 ModuleLoader 도입 시 다룬다).
	std::string normalizedPath = std::filesystem::weakly_canonical(path).string();

	// ModuleLoader가 나중에 도입되면 순환 import 감지 책임은 그쪽으로 옮겨간다.
	// 지금은 Tokenizer가 직접 재귀적으로 파일을 이어붙이기 때문에, 스택 오버플로우를
	// 막기 위한 최소한의 가드만 여기 둔다.
	if (std::find(activeImports.begin(), activeImports.end(), normalizedPath) != activeImports.end()) {
		throw std::runtime_error("Circular import detected: " + normalizedPath + " at line " + std::to_string(line));
	}

	std::string fileSource = ReadFile(path, line);

	activeImports.push_back(normalizedPath);

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
