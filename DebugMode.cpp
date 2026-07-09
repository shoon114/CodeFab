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

	TokenList tokenList = SourceFileLoader::Tokenize(content);
	AssemblerUnit assembler;
	CheckerUnit checker;
	ExecutorUnit executor;
	DebugSession session(SourceFileLoader::SplitIntoLines(content), executor);
	executor.SetObserver(&session);

	try {
		std::unique_ptr<SyntaxNode> tree = assembler.Parse(tokenList);
		checker.Check(tree.get());
		executor.Execute(*tree);
		std::cout << "Program finished." << std::endl;
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}
