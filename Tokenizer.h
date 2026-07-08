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
	std::vector<Token> ScanWords();
	void ClassifyToken(Token& token);
};