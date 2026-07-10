#include "FileMode.h"
#include "SourceFileLoader.h"
#include "AssemblerUnit.h"
#include "CheckerUnit.h"
#include "ExecutorUnit.h"
#include "SyntaxNode.h"
#include <iostream>
#include <memory>

FileMode::FileMode(std::string path) : path(std::move(path)) {
}

int FileMode::Run() {
	std::string content;
	if (!SourceFileLoader::LoadFile(path, content)) {
		std::cerr << "File not found: " << path << std::endl;
		return 1;
	}

	AssemblerUnit assembler;
	CheckerUnit checker;
	ExecutorUnit executor;

	try {
		TokenList tokenList = SourceFileLoader::Tokenize(content);
		std::unique_ptr<SyntaxNode> tree = assembler.Parse(tokenList);
		// 정적 오류가 있으면 ReportError가 이미 stderr에 보고했으므로, 잘못된
		// 트리를 실행해 런타임 오류를 또 내지 않도록 여기서 멈춘다.
		if (!checker.Check(tree.get())) {
			return 1;
		}
		executor.Execute(*tree);
	} catch (const std::exception& e) {
		// 기존 ExecutorUnit의 모든 throw std::runtime_error가 이미 메시지에
		// "at line N"을 포함하므로 별도 가공 없이 그대로 출력한다.
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}
