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
	std::vector<std::string> words;
	std::string word = "";

	for (size_t i = 0; i < originalCode.size(); i++)
	{
		char c = originalCode[i];

		if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
		{
			if (word != "") {
				words.push_back(word);
				word = "";
			}
			continue;
		}

		if (c == '"')
		{
			if (word != "") {
				words.push_back(word);
				word = "";
			}
			std::string str = "\"";
			i++;
			while (i < originalCode.size() && originalCode[i] != '"')
			{
				str += originalCode[i];
				i++;
			}
			str += "\"";
			words.push_back(str);
			continue;
		}

		if (c == '=' || c == '!' || c == '<' || c == '>')
		{
			if (word != "") {
				words.push_back(word);
				word = "";
			}
			if (i + 1 < originalCode.size() && originalCode[i + 1] == '=')
			{
				std::string op = "";
				op += c;
				op += '=';
				words.push_back(op);
				i++;
				continue;
			}
			else
			{
				std::string op = "";
				op += c;
				words.push_back(op);
				continue;
			}
		}

		if (c == '(' || c == ')' || c == '{' || c == '}' || c == ';' || c == ',' || c == '+' || c == '-' || c == '*' || c == '/' || c == '%')
		{
			if (word != "") {
				words.push_back(word);
				word = "";
			}
			std::string op = "";
			op += c;
			words.push_back(op);
			continue;
		}

		word += c;
	}

	if (word != "")
	{
		words.push_back(word);
	}

	return words;
}

TokenList Tokenizer::CreateTokenForCode()
{
	return TokenList();
}
