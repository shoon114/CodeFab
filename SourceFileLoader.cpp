#include "SourceFileLoader.h"
#include "Tokenizer.h"
#include <fstream>
#include <iostream>
#include <sstream>

bool SourceFileLoader::LoadFile(const std::string& path, std::string& outContent) {
	std::ifstream file(path);
	if (!file) {
		return false;
	}
	std::ostringstream contentStream;
	contentStream << file.rdbuf();
	outContent = contentStream.str();
	return true;
}

std::vector<std::string> SourceFileLoader::SplitIntoLines(const std::string& content) {
	std::vector<std::string> lines;
	std::istringstream lineStream(content);
	std::string line;
	while (std::getline(lineStream, line)) {
		lines.push_back(line);
	}
	return lines;
}

TokenList SourceFileLoader::Tokenize(const std::string& source) {
	Tokenizer tokenizer;
	std::istringstream sourceInput(source);
	std::streambuf* originalCinBuffer = std::cin.rdbuf(sourceInput.rdbuf());
	tokenizer.GetCodeFromUser();
	std::cin.rdbuf(originalCinBuffer);
	return tokenizer.CreateTokenForCode();
}
