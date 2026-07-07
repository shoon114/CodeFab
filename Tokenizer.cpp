#include "Tokenizer.h"
#include <iostream>
#include <sstream>

void Tokenizer::GetCodeFromUser()
{
	std::ostringstream buffer;
	buffer << std::cin.rdbuf();
	originalCode = buffer.str();
}

std::vector<std::string> Tokenizer::SplitIntoWords()
{
	return std::vector<std::string>();
}

TokenList Tokenizer::CreateTokenForCode()
{
	return TokenList();
}
