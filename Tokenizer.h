#pragma once
#include "Token.h"
#include <vector>
#include <string>
class Tokenizer {
public:
	void GetCodeFromUser();
	std::vector<std::string> SplitIntoWords();
	virtual TokenList CreateTokenForCode();
private:
	std::string originalCode;

	bool CanExtendToTwoCharOperator(char c);
	bool IsDoubledOperatorStart(char c);
	bool IsSingleCharPunctuation(char c);
	std::vector<Token> ScanWords(const std::string& source);
	void ClassifyToken(Token& token);

	static std::string ReadFile(const std::string& path);
	TokenList TokenizeFileForImport(const std::string& path, std::vector<std::string>& activeImports);
	void ResolveImports(TokenList& tokens, std::vector<std::string>& activeImports);
};