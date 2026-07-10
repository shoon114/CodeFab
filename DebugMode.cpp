#include "DebugMode.h"
#include "SourceFileLoader.h"
#include "AssemblerUnit.h"
#include "CheckerUnit.h"
#include "ExecutorUnit.h"
#include "DebugSession.h"
#include "SyntaxNode.h"
#include <iostream>
#include <memory>

DebugMode::DebugMode(std::string path) : path(std::move(path)) {
}

int DebugMode::Run() {
	std::string content;
	if (!SourceFileLoader::LoadFile(path, content)) {
		std::cerr << "File not found: " << path << std::endl;
		return 1;
	}

	auto lastSlash = path.find_last_of("/\\");
	std::string filename = (lastSlash == std::string::npos) ? path : path.substr(lastSlash + 1);
	std::cout << "[DEBUG] Loading Source Code : " << filename << std::endl;

	TokenList tokenList = SourceFileLoader::Tokenize(content);
	AssemblerUnit assembler;
	CheckerUnit checker;
	ExecutorUnit executor;
	DebugSession session(SourceFileLoader::SplitIntoLines(content), executor);
	executor.SetObserver(&session);

	try {
		std::unique_ptr<SyntaxNode> tree = assembler.Parse(tokenList);
		if (!checker.Check(tree.get())) {
			return 1;
		}
		executor.Execute(*tree);
		std::cout << "\nProgram finished." << std::endl;
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}
	return 0;
}
