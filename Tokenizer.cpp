#include "Tokenizer.h"
#include <iostream>
#include <sstream>
#include <cctype>

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

std::vector<std::string> Tokenizer::SplitIntoWords()
{
	std::vector<std::string> words;
	size_t i = 0;
	const size_t n = originalCode.size();

	while (i < n) {
		char c = originalCode[i];

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
			words.push_back(originalCode.substr(start, i - start));
			continue;
		}

		if (CanExtendToTwoCharOperator(c)) {
			bool isTwoCharOperator = (i + 1 < n && originalCode[i + 1] == '=');
			words.push_back(originalCode.substr(i, isTwoCharOperator ? 2 : 1));
			i += isTwoCharOperator ? 2 : 1;
			continue;
		}

		if (IsSingleCharPunctuation(c)) {
			words.push_back(std::string(1, c));
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
		words.push_back(originalCode.substr(start, i - start));
	}

	return words;
}

TokenList Tokenizer::CreateTokenForCode()
{
	return TokenList();
}
