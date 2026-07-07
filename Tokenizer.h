#pragma once
#include "Token.h"
#include <vector>
#include <string>
class Tokenizer {
public:
	void GetCodeFromUser();
	std::vector<std::string> SplitIntoWords();
	virtual TokenList CreateTokenForCode() { return {}; };
private:
	std::string originalCode;
};